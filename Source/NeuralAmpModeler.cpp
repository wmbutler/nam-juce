#include "NeuralAmpModeler.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>

namespace
{
std::string normaliseGearType(std::string gearType)
{
    std::transform(gearType.begin(), gearType.end(), gearType.begin(), [](unsigned char c)
    {
        return (char) std::tolower(c);
    });

    return gearType;
}

std::string readGearType(const nam::dspData& dspData)
{
    if (!dspData.metadata.is_object())
        return {};

    const auto gearType = dspData.metadata.find("gear_type");
    if (gearType == dspData.metadata.end() || !gearType->is_string())
        return {};

    return normaliseGearType(gearType->get<std::string>());
}

bool gearTypeHasCab(const std::string& gearType)
{
    return gearType == "amp_cab" || gearType == "amp_pedal_cab";
}
} // namespace

NeuralAmpModeler::NeuralAmpModeler()
    : sampleRate(48000.0), samplesPerBlock(512)
{
    mToneStack = std::make_unique<dsp::tone_stack::BasicNamToneStack>();
    nam::activations::Activation::enable_fast_tanh();

    mNoiseGateTrigger.AddListener(&mNoiseGateGain);
}

NeuralAmpModeler::~NeuralAmpModeler() {}

void NeuralAmpModeler::prepare(juce::dsp::ProcessSpec& spec)
{
    this->sampleRate = spec.sampleRate;
    this->samplesPerBlock = spec.maximumBlockSize;

    outputBuffer.setSize(1, spec.maximumBlockSize, false, false, false);
    outputBuffer.clear();

    resetModel();
    mToneStack->Reset(this->sampleRate, this->samplesPerBlock);

    mNoiseGateTrigger.SetSampleRate(this->sampleRate);
}

void NeuralAmpModeler::processBlock(juce::AudioBuffer<float>& buffer)
{
    this->applyDSPStaging();
    this->updateParameters();

    const int numSamples = buffer.getNumSamples();

    if (outputBuffer.getNumSamples() < numSamples)
    {
        outputBuffer.setSize(1, numSamples, false, false, true);
        samplesPerBlock = numSamples;

        if (mModel != nullptr)
            mModel->Reset(sampleRate, numSamples);

        if (mStagedModel != nullptr)
            mStagedModel->Reset(sampleRate, numSamples);
    }

    auto* channelDataLeft = buffer.getWritePointer(0);
    auto* channelDataRight = buffer.getWritePointer(1);
    auto* outputData = outputBuffer.getWritePointer(0);

    float** inputPointer = &channelDataLeft;
    float** outputPointer = &outputData;
    float** processedOutput;
    float** triggerOutput = inputPointer;

    if (noiseGateActive) // Process gate trigger
        triggerOutput = mNoiseGateTrigger.Process(inputPointer, 1, buffer.getNumSamples());

    if (mModel != nullptr)
    {
        // Input Gain
        buffer.applyGain(dB_to_linear(params[Parameters::kInputLevel]->load()));

        // WaveNet resizes/zeroes work matrices whenever num_frames changes; host callbacks often
        // use variable block sizes. Fixed-size chunks keep frames stable (skip when resampling so
        // Lanczos state sees one block per host callback).
        if (mModel->usesInternalResampling())
        {
            mModel->process(*inputPointer, *outputPointer, numSamples);
            mModel->finalize_(numSamples);
        }
        else
        {
            int pos = 0;
            while (pos < numSamples)
            {
                const int n = juce::jmin(kNamInferenceFrames, numSamples - pos);
                mModel->process(*inputPointer + pos, *outputPointer + pos, n);
                mModel->finalize_(n);
                pos += n;
            }
        }

        // Normalize loudness
        if (this->outputNormalized)
            normalizeOutput(outputPointer, 1, buffer.getNumSamples());


        processedOutput = outputPointer;
    }
    else
    {
        processedOutput = inputPointer;
    }

    // Apply the noise gate
    float** gateGainOutput = noiseGateActive ? mNoiseGateGain.Process(processedOutput, 1, buffer.getNumSamples()) : processedOutput;

    // Tone Stack
    float** toneStackOutPointers = toneStackActive ? mToneStack->Process(gateGainOutput, 1, buffer.getNumSamples()) : gateGainOutput;

    doDualMono(buffer, toneStackOutPointers);

    // Output Gain
    buffer.applyGain(dB_to_linear(params[Parameters::kOutputLevel]->load()));
}

bool NeuralAmpModeler::loadModel(const std::string modelPath)
{
    try
    {
        auto dspPath = std::filesystem::u8path(modelPath);
        nam::dspData dspData;
        std::unique_ptr<nam::DSP> model = nam::get_dsp(dspPath, dspData);
        std::unique_ptr<ResamplingNAM> temp = std::make_unique<ResamplingNAM>(std::move(model), this->sampleRate);

        const int resetBlock = juce::jmax(32, this->samplesPerBlock);
        temp->Reset(this->sampleRate, resetBlock);

        mStagedModel = std::move(temp);
        lastGearType = readGearType(dspData);
        lastModelCabBakedIn = gearTypeHasCab(lastGearType);

        return true;
    }
    catch (const std::exception& e)
    {
        if (mStagedModel != nullptr)
        {
            mStagedModel = nullptr;
        }

        lastGearType.clear();
        lastModelCabBakedIn = false;

        std::cout << "woops" << std::endl;
        std::cerr << "Failed to read DSP module" << std::endl;
        std::cerr << e.what() << std::endl;

        return false;
    }
}

bool NeuralAmpModeler::isModelLoaded()
{
    return this->modelLoaded;
}

void NeuralAmpModeler::clearModel()
{
    this->shouldRemoveModel = true;
    lastGearType.clear();
    lastModelCabBakedIn = false;
}

void NeuralAmpModeler::applyDSPStaging()
{
    // Remove marked modules
    if (shouldRemoveModel)
    {
        mModel = nullptr;
        shouldRemoveModel = false;
        //_UpdateLatency();
    }

    // Move things from staged to live
    if (mStagedModel != nullptr)
    {
        // Move from staged to active DSP
        mModel = std::move(mStagedModel);
        mStagedModel = nullptr;
        modelLoaded = true;
        //_UpdateLatency();
    }
}

void NeuralAmpModeler::resetModel()
{
    if (mStagedModel != nullptr)
        mStagedModel->Reset(this->sampleRate, this->samplesPerBlock);

    else if (mModel != nullptr)
        mModel->Reset(this->sampleRate, this->samplesPerBlock);
}

void NeuralAmpModeler::normalizeOutput(float** input, int numChannels, int numSamples)
{
    if (!mModel)
        return;
    if (!mModel->HasLoudness())
        return;

    const double loudness = mModel->GetLoudness();
    const double targetLoudness = -18.0;
    const double gain = pow(10.0, (targetLoudness - loudness) / 20.0);

    for (int c = 0; c < numChannels; c++)
    {
        for (int f = 0; f < numSamples; f++)
        {
            input[c][f] *= gain;
        }
    }
}

void NeuralAmpModeler::updateParameters()
{
    outputNormalized = bool(params[Parameters::kOutNorm]->load());

    // Tone Stack
    toneStackActive = bool(params[Parameters::kEQActive]->load());

    // Noise Gate
    noiseGateActive = int(params[Parameters::kNoiseGateThreshold]->load()) < -100 ? false : true;

    if (toneStackActive)
    {
        mToneStack->SetParam("bass", params[Parameters::kToneBass]->load());
        mToneStack->SetParam("middle", params[Parameters::kToneMid]->load());
        mToneStack->SetParam("treble", params[Parameters::kToneTreble]->load());
    }

    if (noiseGateActive)
    {
        const dsp::noise_gate::TriggerParams triggerParams(
            this->ns_time, params[Parameters::kNoiseGateThreshold]->load(), this->ns_ratio, this->ns_openTime, this->ns_holdTime, this->ns_closeTime);

        mNoiseGateTrigger.SetParams(triggerParams);
    }
}

void NeuralAmpModeler::createParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& parameters)
{
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("INPUT_ID", "INPUT", -20.0f, 20.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("NGATE_ID", "NGATE", -101.0f, 0.0f, -80.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("BASS_ID", "BASS", 0.0f, 10.0f, 5.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("MIDDLE_ID", "MIDDLE", 0.0f, 10.0f, 5.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("TREBLE_ID", "TREBLE", 0.0f, 10.0f, 5.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("OUTPUT_ID", "OUTPUT", -40.0f, 40.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterBool>("TONE_STACK_ON_ID", "TONE_STACK_ON", true, "TONE_STACK_ON"));
    parameters.push_back(std::make_unique<juce::AudioParameterBool>("NORMALIZE_ID", "NORMALIZE", false, "NORMALIZE"));

    DBG("NAM Parameters Created!");
}

void NeuralAmpModeler::hookParameters(juce::AudioProcessorValueTreeState& apvts)
{
    params[Parameters::kInputLevel] = apvts.getRawParameterValue("INPUT_ID");
    params[Parameters::kNoiseGateThreshold] = apvts.getRawParameterValue("NGATE_ID");
    params[Parameters::kToneBass] = apvts.getRawParameterValue("BASS_ID");
    params[Parameters::kToneMid] = apvts.getRawParameterValue("MIDDLE_ID");
    params[Parameters::kToneTreble] = apvts.getRawParameterValue("TREBLE_ID");
    params[Parameters::kOutputLevel] = apvts.getRawParameterValue("OUTPUT_ID");
    params[Parameters::kEQActive] = apvts.getRawParameterValue("TONE_STACK_ON_ID");
    params[Parameters::kOutNorm] = apvts.getRawParameterValue("NORMALIZE_ID");

    DBG("NAM Parameters Hooked!");
}

double NeuralAmpModeler::dB_to_linear(double db_value)
{
    return std::pow(10.0, db_value / 20.0);
}

void NeuralAmpModeler::doDualMono(juce::AudioBuffer<float>& mainBuffer, float** input)
{
    auto channelDataLeft = mainBuffer.getWritePointer(0);
    auto channelDataRight = mainBuffer.getWritePointer(1);

    for (int sample = 0; sample < mainBuffer.getNumSamples(); ++sample)
    {
        channelDataRight[sample] = input[0][sample];
        channelDataLeft[sample] = input[0][sample];
    }
}