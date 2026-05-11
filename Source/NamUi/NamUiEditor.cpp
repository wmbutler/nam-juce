#include "PluginProcessor.h"
#include "NamUiEditor.h"
#include "NamUiFonts.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#if JucePlugin_Build_Standalone
    #include "../CustomStandaloneWindow/CustomStandaloneFilterWindow.h"
#endif

namespace NamUi
{

juce::Font NamUiEditor::EditorLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    juce::ignoreUnused(buttonHeight);
    return Fonts::shareTechMono(11.f, 0.1f);
}

juce::Font NamUiEditor::EditorLookAndFeel::getPopupMenuFont()
{
    return Fonts::shareTechMono(11.f, 0.f);
}

NamUiEditor::~NamUiEditor()
{
    setLookAndFeel(nullptr);
}

namespace
{
static constexpr int kRowGap = 2;
static constexpr int kBrowserPadX = 16;
static constexpr int kIoStripTop = 12;
static constexpr int kIoStripHeight = 56;
static constexpr int kIoGap = 10;
static constexpr int kPresetGap = 8;
static constexpr int kPresetTop = kIoStripTop + kIoStripHeight + kPresetGap;
/** Total height of the four stacked browser rows (capture/IR collections + files). */
static constexpr int kBrowserClusterHeight =
    4 * NamUiBrowserRow::kDefaultHeight + 3 * kRowGap;
static constexpr int kBrowserTop = kPresetTop + NamUiPresetPanel::kDefaultHeight + kPresetGap;
static constexpr int kMainStripTop = kBrowserTop + kBrowserClusterHeight + kPresetGap;
static constexpr int kMainStripHeight = 300;
static constexpr auto* kToneDirectorySettingKey = "toneDirectory";
static constexpr auto* kPresetDirectoryName = "Presets";
static constexpr auto* kPresetManifestName = "manifest.json";

static double normalise(float value, float minValue, float maxValue)
{
    return juce::jlimit(0.0, 1.0, (double)((value - minValue) / (maxValue - minValue)));
}

static float parameterValue(const juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float fallback)
{
    if (auto* value = apvts.getRawParameterValue(id))
        return value->load();

    return fallback;
}

static void setParameterValue(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float value)
{
    if (auto* parameter = apvts.getParameter(id))
    {
        const float normalised = parameter->convertTo0to1(value);
        if (std::abs(parameter->getValue() - normalised) < 1.0e-6f)
            return;

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(normalised);
        parameter->endChangeGesture();
    }
}

static int wrapIndex(int index, int size)
{
    if (size <= 0)
        return 0;

    return (index + size) % size;
}

static juce::String namDirPrompt()
{
    return "File > Settings > Set NAM Directory";
}

static std::vector<juce::File> findSortedChildren(const juce::File& directory, int whatToFind, const juce::String& wildcard)
{
    std::vector<juce::File> result;

    if (!directory.isDirectory())
        return result;

    for (const auto& file : directory.findChildFiles(whatToFind, false, wildcard))
        result.push_back(file);

    std::sort(result.begin(), result.end(), [](const juce::File& a, const juce::File& b)
    {
        return a.getFileName().compareNatural(b.getFileName()) < 0;
    });

    return result;
}

static bool sameFile(const juce::File& a, const juce::File& b)
{
    return a.getFullPathName() == b.getFullPathName();
}

static std::vector<juce::File> findCollectionDirectories(const juce::File& root, const juce::String& looseFileWildcard)
{
    auto collections = findSortedChildren(root, juce::File::findDirectories, "*");

    if (!findSortedChildren(root, juce::File::findFiles, looseFileWildcard).empty())
        collections.insert(collections.begin(), root);

    return collections;
}

static juce::StringArray fileNames(const std::vector<juce::File>& files)
{
    juce::StringArray names;

    for (const auto& file : files)
        names.add(file.getFileName());

    return names;
}

static juce::StringArray collectionNames(const std::vector<juce::File>& collections, const juce::File& root)
{
    juce::StringArray names;

    for (const auto& collection : collections)
        names.add(sameFile(collection, root) ? "Standalone" : collection.getFileName());

    return names;
}

static bool isFullRigCapture(const juce::File& captureFile)
{
    const auto sidecar = captureFile.withFileExtension(".json");

    if (sidecar.existsAsFile())
    {
        if (auto parsed = juce::JSON::parse(sidecar); parsed.isObject())
        {
            if (auto* object = parsed.getDynamicObject())
                if (object->getProperty("type").toString().equalsIgnoreCase("full_rig"))
                    return true;
        }
    }

    const auto parentName = captureFile.getParentDirectory().getFileName();
    return parentName.containsIgnoreCase("Full Rig")
        || parentName.containsIgnoreCase("Combo")
        || parentName.containsIgnoreCase(" with ");
}

static juce::String readToneDirectoryFromStandaloneProperties()
{
    juce::PropertiesFile::Options options;

    options.applicationName = juce::CharPointer_UTF8(JucePlugin_Name);
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
#if JUCE_LINUX || JUCE_BSD
    options.folderName = "~/.config";
#else
    options.folderName = "";
#endif

    return juce::PropertiesFile(options).getValue(kToneDirectorySettingKey);
}

static juce::File presetDirectoryFor(const juce::File& namRoot)
{
    return namRoot.getChildFile(kPresetDirectoryName);
}

static juce::File presetManifestFor(const juce::File& namRoot)
{
    return presetDirectoryFor(namRoot).getChildFile(kPresetManifestName);
}

static juce::String normalisedRelativePath(const juce::File& root, const juce::File& file)
{
    return file.getRelativePathFrom(root).replaceCharacter('\\', '/');
}

static juce::File resolveRelativePath(const juce::File& root, juce::String relativePath)
{
    return root.getChildFile(relativePath.replaceCharacter('\\', '/'));
}

static juce::String stringProperty(const juce::DynamicObject& object, const juce::Identifier& property,
                                   const juce::String& fallback = {})
{
    const auto value = object.getProperty(property);
    return value.isVoid() ? fallback : value.toString();
}

static double doubleProperty(const juce::DynamicObject& object, const juce::Identifier& property, double fallback)
{
    const auto value = object.getProperty(property);
    return value.isVoid() ? fallback : (double) value;
}

static bool boolProperty(const juce::DynamicObject& object, const juce::Identifier& property, bool fallback)
{
    const auto value = object.getProperty(property);
    return value.isVoid() ? fallback : (bool) value;
}

static bool approximatelyEqual(double a, double b)
{
    return std::abs(a - b) < 0.0001;
}

static bool sameControlState(const NamUiMainControlsStrip::ControlState& a,
                             const NamUiMainControlsStrip::ControlState& b)
{
    return approximatelyEqual(a.inputLevel, b.inputLevel)
        && approximatelyEqual(a.outputLevel, b.outputLevel)
        && approximatelyEqual(a.bass, b.bass)
        && approximatelyEqual(a.mid, b.mid)
        && approximatelyEqual(a.treble, b.treble)
        && approximatelyEqual(a.gateOpen, b.gateOpen)
        && approximatelyEqual(a.gateClose, b.gateClose)
        && a.toneActive == b.toneActive
        && a.noiseGateActive == b.noiseGateActive;
}

static bool sameSnapshot(const NamUiEditor::PresetSnapshot& a, const NamUiEditor::PresetSnapshot& b)
{
    return a.capturePath == b.capturePath
        && a.irPath == b.irPath
        && a.captureType == b.captureType
        && a.irCollection == b.irCollection
        && sameControlState(a.controls, b.controls)
        && a.captureActive == b.captureActive
        && a.irActive == b.irActive;
}

static bool readPresetSnapshot(const juce::File& presetFile, NamUiEditor::PresetSnapshot& snapshot)
{
    if (!presetFile.existsAsFile())
        return false;

    auto parsed = juce::JSON::parse(presetFile);
    if (!parsed.isObject())
        return false;

    auto* object = parsed.getDynamicObject();
    if (object == nullptr)
        return false;

    snapshot.id = stringProperty(*object, "id", presetFile.getFileNameWithoutExtension());
    snapshot.name = stringProperty(*object, "name", snapshot.id);
    snapshot.capturePath = stringProperty(*object, "capture");
    snapshot.irPath = stringProperty(*object, "ir");
    snapshot.captureType = stringProperty(*object, "capture_type", "amp_head");
    snapshot.irCollection = stringProperty(*object, "ir_collection");
    snapshot.controls.bass = doubleProperty(*object, "bass", 0.5);
    snapshot.controls.mid = doubleProperty(*object, "mid", 0.5);
    snapshot.controls.treble = doubleProperty(*object, "treble", 0.5);
    snapshot.controls.gateOpen = doubleProperty(*object, "gate_open", 0.0);
    snapshot.controls.gateClose = doubleProperty(*object, "gate_close", 1.0);
    snapshot.controls.inputLevel = doubleProperty(*object, "input_level", 0.5);
    snapshot.controls.outputLevel = doubleProperty(*object, "output_level", 0.5);
    snapshot.controls.toneActive = boolProperty(*object, "tone_active", false);
    snapshot.controls.noiseGateActive = boolProperty(*object, "gate_active", false);
    snapshot.irActive = boolProperty(*object, "ir_active", false);
    snapshot.captureActive = boolProperty(*object, "capture_active", false);
    return true;
}

static juce::var presetSnapshotToVar(const NamUiEditor::PresetSnapshot& snapshot)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty("id", snapshot.id);
    object->setProperty("name", snapshot.name);
    object->setProperty("capture", snapshot.capturePath);
    object->setProperty("ir", snapshot.irPath);
    object->setProperty("capture_type", snapshot.captureType);
    object->setProperty("ir_collection", snapshot.irCollection);
    object->setProperty("bass", snapshot.controls.bass);
    object->setProperty("mid", snapshot.controls.mid);
    object->setProperty("treble", snapshot.controls.treble);
    object->setProperty("gate_open", snapshot.controls.gateOpen);
    object->setProperty("gate_close", snapshot.controls.gateClose);
    object->setProperty("input_level", snapshot.controls.inputLevel);
    object->setProperty("output_level", snapshot.controls.outputLevel);
    object->setProperty("tone_active", snapshot.controls.toneActive);
    object->setProperty("gate_active", snapshot.controls.noiseGateActive);
    object->setProperty("ir_active", snapshot.irActive);
    object->setProperty("capture_active", snapshot.captureActive);
    return juce::var(object.release());
}

static bool writePresetSnapshot(const juce::File& presetFile, const NamUiEditor::PresetSnapshot& snapshot)
{
    if (presetFile.getParentDirectory().createDirectory().failed())
        return false;

    return presetFile.replaceWithText(juce::JSON::toString(presetSnapshotToVar(snapshot), false));
}

static bool writePresetManifest(const juce::File& namRoot, const std::vector<NamUiEditor::PresetEntry>& entries)
{
    auto array = juce::Array<juce::var>();

    for (const auto& entry : entries)
    {
        auto object = std::make_unique<juce::DynamicObject>();
        object->setProperty("id", entry.id);
        object->setProperty("name", entry.name);
        array.add(juce::var(object.release()));
    }

    const auto manifest = presetManifestFor(namRoot);
    if (manifest.getParentDirectory().createDirectory().failed())
        return false;

    return manifest.replaceWithText(juce::JSON::toString(juce::var(array), false));
}
} // namespace

NamUiMainControlsStrip::ControlState NamUiEditor::makeMainControlStateFromProcessor() const
{
    const auto& apvts = audioProcessor.apvts;
    const float inputDb = parameterValue(apvts, "INPUT_ID", 0.f);
    const float outputDb = parameterValue(apvts, "OUTPUT_ID", 0.f);
    const float gateDb = parameterValue(apvts, "NGATE_ID", -101.f);

    NamUiMainControlsStrip::ControlState state;
    state.inputLevel = normalise(inputDb, -20.f, 20.f);
    // The mock output level knob is +/-20 dB even though the legacy APVTS range is wider.
    state.outputLevel = normalise(juce::jlimit(-20.f, 20.f, outputDb), -20.f, 20.f);
    state.bass = normalise(parameterValue(apvts, "BASS_ID", 5.f), 0.f, 10.f);
    state.mid = normalise(parameterValue(apvts, "MIDDLE_ID", 5.f), 0.f, 10.f);
    state.treble = normalise(parameterValue(apvts, "TREBLE_ID", 5.f), 0.f, 10.f);
    state.gateOpen = normalise(juce::jlimit(-100.f, 0.f, gateDb), -100.f, 0.f);
    state.gateClose = 1.0;
    state.toneActive = parameterValue(apvts, "TONE_STACK_ON_ID", 1.f) >= 0.5f;
    state.noiseGateActive = gateDb > -100.5f;
    return state;
}

void NamUiEditor::applyMainControlStateToProcessor(const NamUiMainControlsStrip::ControlState& state)
{
    auto& apvts = audioProcessor.apvts;

    setParameterValue(apvts, "INPUT_ID", (float)juce::jmap(state.inputLevel, 0.0, 1.0, -20.0, 20.0));
    setParameterValue(apvts, "OUTPUT_ID", (float)juce::jmap(state.outputLevel, 0.0, 1.0, -20.0, 20.0));
    setParameterValue(apvts, "BASS_ID", (float)juce::jmap(state.bass, 0.0, 1.0, 0.0, 10.0));
    setParameterValue(apvts, "MIDDLE_ID", (float)juce::jmap(state.mid, 0.0, 1.0, 0.0, 10.0));
    setParameterValue(apvts, "TREBLE_ID", (float)juce::jmap(state.treble, 0.0, 1.0, 0.0, 10.0));
    setParameterValue(apvts, "TONE_STACK_ON_ID", state.toneActive ? 1.f : 0.f);

    const auto gateThreshold = state.noiseGateActive ? (float)juce::jmap(state.gateOpen, 0.0, 1.0, -100.0, 0.0)
                                                     : -101.f;
    setParameterValue(apvts, "NGATE_ID", gateThreshold);
}

NamUiEditor::NamUiEditor(NamJUCEAudioProcessor& processor)
    : audioProcessor(processor)
{
    juce::ignoreUnused(audioProcessor);
    setOpaque(true);
    setWantsKeyboardFocus(true);
    setLookAndFeel(&editorLookAndFeel);

    addAndMakeVisible(inputSource);
    addAndMakeVisible(outputSource);

#if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
    {
        inputSource.attachToAudioDeviceManager(&holder->deviceManager);
        outputSource.attachToAudioDeviceManager(&holder->deviceManager);
        holder->getMuteInputValue().setValue(false);
    }
#else
    inputSource.setSelectedValue("Host input");
    outputSource.setSelectedValue("Host output");
#endif

    addAndMakeVisible(presetPanel);
    presetPanel.setOnPrev([this]
    {
        const int n = (int) presetEntries.size();
        if (n <= 1)
            return;
        loadPresetAt((presetIndex - 1 + n) % n, -1);
    });
    presetPanel.setOnNext([this]
    {
        const int n = (int) presetEntries.size();
        if (n <= 1)
            return;
        loadPresetAt((presetIndex + 1) % n, 1);
    });

    presetPanel.setOnSave([this]
    {
        writeCurrentPreset();
    });
    presetPanel.setOnNewPreset([this](juce::String name)
    {
        createPreset(std::move(name));
    });
    presetPanel.setOnRenamePreset([this](juce::String name)
    {
        renameCurrentPreset(std::move(name));
    });
    presetPanel.setOnDeletePreset([this]
    {
        deleteCurrentPreset();
    });
    presetPanel.setOnMovePreset([this](int from, int to)
    {
        movePreset(from, to);
    });

    syncPresetPanel();

    addAndMakeVisible(browserCaptureCollection);
    addAndMakeVisible(browserCaptureFile);
    addAndMakeVisible(browserIrCollection);
    addAndMakeVisible(browserIrFile);

    addAndMakeVisible(mainControlsStrip);
    mainControlsStrip.setMeterSources(&audioProcessor.getMeterInSource(), &audioProcessor.getMeterOutSource());
    mainControlsStrip.setControlState(makeMainControlStateFromProcessor());
    mainControlsStrip.setOnControlStateChanged([this](const NamUiMainControlsStrip::ControlState& state)
    {
        applyMainControlStateToProcessor(state);
    });
    mainControlsStrip.setOnMuteChanged([this](bool inputMuted, bool outputMuted)
    {
        audioProcessor.setInputMuted(inputMuted);
        audioProcessor.setOutputMuted(outputMuted);
        inputSource.setChannelActive(!inputMuted);
        outputSource.setChannelActive(!outputMuted);
    });
    mainControlsStrip.setOnControlsChanged([this]
    {
        markPresetDirty();
    });

    browserCaptureCollection.setOnPrev([this]
    {
        if (captureCollectionDirs.empty())
            return;

        captureCollectionIndex = wrapIndex(captureCollectionIndex - 1, (int) captureCollectionDirs.size());
        captureFileIndex = 0;
        markPresetDirty();
        refreshBrowserRows();
    });
    browserCaptureCollection.setOnNext([this]
    {
        if (captureCollectionDirs.empty())
            return;

        captureCollectionIndex = wrapIndex(captureCollectionIndex + 1, (int) captureCollectionDirs.size());
        captureFileIndex = 0;
        markPresetDirty();
        refreshBrowserRows();
    });

    browserCaptureFile.setActivateButtonText("CAPTURE");
    browserCaptureFile.setOnActivate([this]
    {
        if (captureFiles.empty())
            return;

        captureActive = !captureActive;
        audioProcessor.setCaptureActive(captureActive);
        markPresetDirty();
        refreshBrowserRows();
    });
    browserCaptureFile.setOnPrev([this]
    {
        if (captureFiles.empty())
            return;

        captureFileIndex = wrapIndex(captureFileIndex - 1, (int) captureFiles.size());
        markPresetDirty();
        refreshBrowserRows();
    });
    browserCaptureFile.setOnNext([this]
    {
        if (captureFiles.empty())
            return;

        captureFileIndex = wrapIndex(captureFileIndex + 1, (int) captureFiles.size());
        markPresetDirty();
        refreshBrowserRows();
    });

    browserIrCollection.setOnPrev([this]
    {
        if (loadedCaptureIsFullRig || irCollectionDirs.empty())
            return;

        irCollectionIndex = wrapIndex(irCollectionIndex - 1, (int) irCollectionDirs.size());
        irFileIndex = 0;
        markPresetDirty();
        refreshBrowserRows();
    });
    browserIrCollection.setOnNext([this]
    {
        if (loadedCaptureIsFullRig || irCollectionDirs.empty())
            return;

        irCollectionIndex = wrapIndex(irCollectionIndex + 1, (int) irCollectionDirs.size());
        irFileIndex = 0;
        markPresetDirty();
        refreshBrowserRows();
    });

    browserIrFile.setActivateButtonText("IR (CAB)");
    browserIrFile.setOnActivate([this]
    {
        if (loadedCaptureIsFullRig || irFiles.empty())
            return;

        irActive = !irActive;
        audioProcessor.setIrActive(irActive);
        setParameterValue(audioProcessor.apvts, "CAB_ON_ID", irActive ? 1.f : 0.f);
        markPresetDirty();
        refreshBrowserRows();
    });
    browserIrFile.setOnPrev([this]
    {
        if (loadedCaptureIsFullRig || irFiles.empty())
            return;

        irFileIndex = wrapIndex(irFileIndex - 1, (int) irFiles.size());
        markPresetDirty();
        refreshBrowserRows();
    });
    browserIrFile.setOnNext([this]
    {
        if (loadedCaptureIsFullRig || irFiles.empty())
            return;

        irFileIndex = wrapIndex(irFileIndex + 1, (int) irFiles.size());
        markPresetDirty();
        refreshBrowserRows();
    });

    refreshNamDirectoryFromSettings();
    startTimerHz(2);
}

bool NamUiEditor::handlePresetArrowKey(const juce::KeyPress& key)
{
    if (presetPanel.isNamingActive())
        return false;

    const int n = (int) presetEntries.size();
    if (n <= 1)
        return false;

    if (key == juce::KeyPress::leftKey)
    {
        loadPresetAt((presetIndex - 1 + n) % n, -1);
        return true;
    }

    if (key == juce::KeyPress::rightKey)
    {
        loadPresetAt((presetIndex + 1) % n, 1);
        return true;
    }

    return false;
}

bool NamUiEditor::keyPressed(const juce::KeyPress& key)
{
    return handlePresetArrowKey(key);
}

void NamUiEditor::mouseDown(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    grabKeyboardFocus();
}

void NamUiEditor::refreshPresetLibrary()
{
    presetEntries.clear();
    hasLoadedPresetSnapshot = false;
    presetDirty = false;
    presetIndex = 0;

    if (!namRootDirectory.isDirectory())
    {
        syncPresetPanel();
        return;
    }

    const auto presetDir = presetDirectoryFor(namRootDirectory);
    presetDir.createDirectory();

    const auto manifest = presetManifestFor(namRootDirectory);
    if (manifest.existsAsFile())
    {
        if (auto parsed = juce::JSON::parse(manifest); parsed.isArray())
        {
            if (auto* array = parsed.getArray())
            {
                for (const auto& item : *array)
                {
                    if (auto* object = item.getDynamicObject())
                    {
                        PresetEntry entry;
                        entry.id = stringProperty(*object, "id");
                        entry.name = stringProperty(*object, "name", entry.id);

                        if (entry.id.isNotEmpty() && presetDir.getChildFile(entry.id + ".json").existsAsFile())
                            presetEntries.push_back(std::move(entry));
                    }
                }
            }
        }
    }

    if (presetEntries.empty())
    {
        for (const auto& file : findSortedChildren(presetDir, juce::File::findFiles, "*.json"))
        {
            if (file.getFileName().equalsIgnoreCase(kPresetManifestName))
                continue;

            PresetSnapshot snapshot;
            if (!readPresetSnapshot(file, snapshot))
                continue;

            presetEntries.push_back({ snapshot.id, snapshot.name });
        }

        if (!presetEntries.empty())
            writePresetManifest(namRootDirectory, presetEntries);
    }

    if (!presetEntries.empty())
        loadPresetAt(juce::jlimit(0, (int) presetEntries.size() - 1, presetIndex), 0);
    else
        syncPresetPanel();
}

void NamUiEditor::syncPresetPanel()
{
    const int n = (int) presetEntries.size();

    presetPanel.setHasLoadedPreset(hasLoadedPresetSnapshot && n > 0);
    presetPanel.setPresetDirty(presetDirty);

    if (n == 0)
    {
        presetPanel.setPresetName("EMPTY PRESET");
        presetPanel.setCounter(0, 0);
        return;
    }

    presetIndex = juce::jlimit(0, n - 1, presetIndex);
    presetPanel.setPresetName(hasLoadedPresetSnapshot ? loadedPresetSnapshot.name : presetEntries[(size_t) presetIndex].name);
    presetPanel.setCounter(presetIndex, n);
}

void NamUiEditor::markPresetDirty()
{
    if (applyingPreset)
        return;

    if (hasLoadedPresetSnapshot)
        presetDirty = !sameSnapshot(makeCurrentPresetSnapshot(loadedPresetSnapshot.id, loadedPresetSnapshot.name), loadedPresetSnapshot);
    else
        presetDirty = true;

    presetPanel.setPresetDirty(presetDirty);
}

void NamUiEditor::loadPresetAt(int index, int direction)
{
    const int n = (int) presetEntries.size();
    if (n == 0 || !namRootDirectory.isDirectory())
        return;

    const int step = direction == 0 ? 1 : direction;
    int candidate = wrapIndex(index, n);

    for (int attempts = 0; attempts < n; ++attempts)
    {
        const auto& entry = presetEntries[(size_t) candidate];
        PresetSnapshot snapshot;
        if (readPresetSnapshot(presetDirectoryFor(namRootDirectory).getChildFile(entry.id + ".json"), snapshot))
        {
            if (snapshot.name.isEmpty())
                snapshot.name = entry.name;

            applyPresetSnapshot(snapshot);
            presetIndex = candidate;
            loadedPresetSnapshot = makeCurrentPresetSnapshot(snapshot.id, snapshot.name);
            hasLoadedPresetSnapshot = true;
            presetDirty = false;
            syncPresetPanel();
            return;
        }

        DBG("NAM preset file missing or invalid: " + entry.id);
        candidate = wrapIndex(candidate + step, n);
    }

    hasLoadedPresetSnapshot = false;
    presetDirty = false;
    syncPresetPanel();
}

void NamUiEditor::writeCurrentPreset()
{
    if (presetEntries.empty() || presetIndex < 0 || presetIndex >= (int) presetEntries.size() || !namRootDirectory.isDirectory())
        return;

    const auto& entry = presetEntries[(size_t) presetIndex];
    const auto snapshot = makeCurrentPresetSnapshot(entry.id, entry.name);
    if (!writePresetSnapshot(presetDirectoryFor(namRootDirectory).getChildFile(entry.id + ".json"), snapshot))
        return;

    loadedPresetSnapshot = snapshot;
    hasLoadedPresetSnapshot = true;
    presetDirty = false;
    writePresetManifest(namRootDirectory, presetEntries);
    syncPresetPanel();
}

void NamUiEditor::createPreset(juce::String name)
{
    if (!namRootDirectory.isDirectory())
        return;

    name = name.trim();
    if (name.isEmpty())
        name = "UNTITLED";

    const auto id = juce::Uuid().toDashedString().removeCharacters("-").substring(0, 8);
    auto snapshot = makeCurrentPresetSnapshot(id, name);

    if (!writePresetSnapshot(presetDirectoryFor(namRootDirectory).getChildFile(id + ".json"), snapshot))
        return;

    presetEntries.push_back({ id, name });
    presetIndex = (int) presetEntries.size() - 1;
    loadedPresetSnapshot = snapshot;
    hasLoadedPresetSnapshot = true;
    presetDirty = false;
    writePresetManifest(namRootDirectory, presetEntries);
    syncPresetPanel();
}

void NamUiEditor::renameCurrentPreset(juce::String name)
{
    if (presetEntries.empty() || presetIndex < 0 || presetIndex >= (int) presetEntries.size())
        return;

    name = name.trim();
    if (name.isEmpty())
        return;

    auto& entry = presetEntries[(size_t) presetIndex];
    entry.name = name;
    auto snapshot = makeCurrentPresetSnapshot(entry.id, entry.name);

    if (!writePresetSnapshot(presetDirectoryFor(namRootDirectory).getChildFile(entry.id + ".json"), snapshot))
        return;

    loadedPresetSnapshot = snapshot;
    hasLoadedPresetSnapshot = true;
    presetDirty = false;
    writePresetManifest(namRootDirectory, presetEntries);
    syncPresetPanel();
}

void NamUiEditor::deleteCurrentPreset()
{
    if (presetEntries.empty() || presetIndex < 0 || presetIndex >= (int) presetEntries.size() || !namRootDirectory.isDirectory())
        return;

    const auto oldIndex = presetIndex;
    const auto id = presetEntries[(size_t) oldIndex].id;
    presetDirectoryFor(namRootDirectory).getChildFile(id + ".json").deleteFile();
    presetEntries.erase(presetEntries.begin() + oldIndex);
    writePresetManifest(namRootDirectory, presetEntries);

    hasLoadedPresetSnapshot = false;
    presetDirty = false;

    if (presetEntries.empty())
    {
        presetIndex = 0;
        syncPresetPanel();
        return;
    }

    presetIndex = juce::jmin(oldIndex, (int) presetEntries.size() - 1);
    loadPresetAt(presetIndex, 0);
}

void NamUiEditor::movePreset(int from, int to)
{
    const int n = (int) presetEntries.size();
    if (from < 0 || to < 0 || from >= n || to >= n || from == to)
        return;

    auto entry = presetEntries[(size_t) from];
    presetEntries.erase(presetEntries.begin() + from);
    presetEntries.insert(presetEntries.begin() + to, std::move(entry));
    presetIndex = to;
    writePresetManifest(namRootDirectory, presetEntries);
    syncPresetPanel();
}

NamUiEditor::PresetSnapshot NamUiEditor::makeCurrentPresetSnapshot(juce::String id, juce::String name) const
{
    PresetSnapshot snapshot;
    snapshot.id = std::move(id);
    snapshot.name = std::move(name);
    snapshot.controls = makeMainControlStateFromProcessor();
    snapshot.captureActive = captureActive;
    snapshot.irActive = irActive;
    snapshot.captureType = loadedCaptureIsFullRig ? "full_rig" : "amp_head";

    if (namRootDirectory.isDirectory() && captureLoaded && !captureFiles.empty())
        snapshot.capturePath = normalisedRelativePath(namRootDirectory, captureFiles[(size_t) captureFileIndex]);

    if (namRootDirectory.isDirectory() && irLoaded && !loadedCaptureIsFullRig && !irFiles.empty())
    {
        snapshot.irPath = normalisedRelativePath(namRootDirectory, irFiles[(size_t) irFileIndex]);
        if (!irCollectionDirs.empty())
            snapshot.irCollection = irCollectionDirs[(size_t) irCollectionIndex].getFileName();
    }

    return snapshot;
}

void NamUiEditor::applyPresetSnapshot(const PresetSnapshot& snapshot)
{
    juce::ScopedValueSetter<bool> guard(applyingPreset, true);

    mainControlsStrip.setControlState(snapshot.controls);
    applyMainControlStateToProcessor(snapshot.controls);

    captureCollectionDirs = findCollectionDirectories(namRootDirectory.getChildFile("Captures"), "*.nam");
    irCollectionDirs = findCollectionDirectories(namRootDirectory.getChildFile("IRs"), "*.wav");

    auto selectFile = [](const juce::File& file, const std::vector<juce::File>& dirs, const juce::String& wildcard,
                         int& dirIndex, int& fileIndex, std::vector<juce::File>& files)
    {
        const auto parent = file.getParentDirectory();
        for (int i = 0; i < (int) dirs.size(); ++i)
        {
            if (dirs[(size_t) i] == parent)
            {
                dirIndex = i;
                files = findSortedChildren(parent, juce::File::findFiles, wildcard);
                for (int j = 0; j < (int) files.size(); ++j)
                {
                    if (files[(size_t) j] == file)
                    {
                        fileIndex = j;
                        return;
                    }
                }
            }
        }
    };

    if (snapshot.capturePath.isNotEmpty())
    {
        const auto captureFile = resolveRelativePath(namRootDirectory, snapshot.capturePath);
        selectFile(captureFile, captureCollectionDirs, "*.nam", captureCollectionIndex, captureFileIndex, captureFiles);
    }

    if (snapshot.irPath.isNotEmpty() && !snapshot.captureType.equalsIgnoreCase("full_rig"))
    {
        const auto irFile = resolveRelativePath(namRootDirectory, snapshot.irPath);
        selectFile(irFile, irCollectionDirs, "*.wav", irCollectionIndex, irFileIndex, irFiles);
    }

    captureActive = snapshot.captureActive;
    irActive = snapshot.captureType.equalsIgnoreCase("full_rig") ? false : snapshot.irActive;
    audioProcessor.setCaptureActive(captureActive);
    audioProcessor.setIrActive(irActive);
    setParameterValue(audioProcessor.apvts, "CAB_ON_ID", irActive ? 1.f : 0.f);
    refreshBrowserRows();
}

void NamUiEditor::paint(juce::Graphics& g)
{
    g.fillAll(colours.bg);
}

void NamUiEditor::timerCallback()
{
    refreshNamDirectoryFromSettings();
}

void NamUiEditor::refreshNamDirectoryFromSettings()
{
    juce::String configuredPath;

#if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
        if (auto* settings = holder->settings.get())
            configuredPath = settings->getValue(kToneDirectorySettingKey);

    if (configuredPath.isEmpty())
        configuredPath = readToneDirectoryFromStandaloneProperties();
#endif

    if (namDirectoryStateInitialised && configuredPath == lastObservedNamDirectoryPath)
    {
        refreshBrowserRows();
        return;
    }

    namDirectoryStateInitialised = true;
    lastObservedNamDirectoryPath = configuredPath;
    namRootDirectory = configuredPath.isNotEmpty() ? juce::File(configuredPath) : juce::File{};
    captureCollectionIndex = 0;
    captureFileIndex = 0;
    irCollectionIndex = 0;
    irFileIndex = 0;
    captureLoaded = false;
    captureActive = false;
    irLoaded = false;
    irActive = false;
    loadedCaptureIsFullRig = false;
    loadedCaptureCollectionIndex = -1;
    loadedCaptureFileIndex = -1;
    refreshBrowserRows();
    refreshPresetLibrary();
}

void NamUiEditor::refreshBrowserRows()
{
    auto showNamDirectoryMissing = [this]
    {
        for (auto* row : {static_cast<NamUiBrowserRow*>(&browserCaptureCollection),
                          &browserCaptureFile, &browserIrCollection, &browserIrFile})
        {
            row->setShowActivateControl(false);
            row->setItems(juce::StringArray(namDirPrompt()));
            row->setBrowseIndex(0);
            row->setBrowseTotal(1);
            row->setRowDisabled(true);
        }
    };

    if (!namRootDirectory.isDirectory())
    {
        captureCollectionDirs.clear();
        captureFiles.clear();
        irCollectionDirs.clear();
        irFiles.clear();
        showNamDirectoryMissing();
        resized();
        return;
    }

    const auto capturesRoot = namRootDirectory.getChildFile("Captures");
    const auto irRoot = namRootDirectory.getChildFile("IRs");
    captureCollectionDirs = findCollectionDirectories(capturesRoot, "*.nam");
    irCollectionDirs = findCollectionDirectories(irRoot, "*.wav");

    captureCollectionIndex = juce::jlimit(0, juce::jmax(0, (int) captureCollectionDirs.size() - 1), captureCollectionIndex);
    irCollectionIndex = juce::jlimit(0, juce::jmax(0, (int) irCollectionDirs.size() - 1), irCollectionIndex);

    captureFiles = captureCollectionDirs.empty()
        ? std::vector<juce::File>{}
        : findSortedChildren(captureCollectionDirs[(size_t) captureCollectionIndex], juce::File::findFiles, "*.nam");
    irFiles = irCollectionDirs.empty()
        ? std::vector<juce::File>{}
        : findSortedChildren(irCollectionDirs[(size_t) irCollectionIndex], juce::File::findFiles, "*.wav");

    captureFileIndex = juce::jlimit(0, juce::jmax(0, (int) captureFiles.size() - 1), captureFileIndex);
    irFileIndex = juce::jlimit(0, juce::jmax(0, (int) irFiles.size() - 1), irFileIndex);

    if (captureFiles.empty())
    {
        captureLoaded = false;
        captureActive = false;
        audioProcessor.setCaptureActive(false);
        loadedCaptureIsFullRig = false;
        loadedCaptureCollectionIndex = -1;
        loadedCaptureFileIndex = -1;
    }
    else
    {
        const auto& selectedCapture = captureFiles[(size_t) captureFileIndex];
        captureLoaded = true;
        loadedCaptureCollectionIndex = captureCollectionIndex;
        loadedCaptureFileIndex = captureFileIndex;
        loadedCaptureIsFullRig = isFullRigCapture(selectedCapture);

        if (audioProcessor.getLastModelPath() != selectedCapture.getFullPathName().toStdString())
            audioProcessor.loadNamModel(selectedCapture);
    }

    if (loadedCaptureIsFullRig || irFiles.empty())
    {
        irLoaded = false;
        irActive = false;
        audioProcessor.setIrActive(false);
        setParameterValue(audioProcessor.apvts, "CAB_ON_ID", 0.f);

        if (loadedCaptureIsFullRig && audioProcessor.getLastIrPath() != "null")
            audioProcessor.clearIR();
    }
    else
    {
        const auto& selectedIr = irFiles[(size_t) irFileIndex];
        irLoaded = true;

        if (audioProcessor.getLastIrPath() != selectedIr.getFullPathName().toStdString())
            audioProcessor.loadImpulseResponse(selectedIr);
    }

    auto configureRow = [](NamUiBrowserRow& row, juce::StringArray items, int index, bool disabled, bool showActivate)
    {
        row.setShowActivateControl(showActivate);
        row.setItems(std::move(items));
        row.setBrowseTotal(0);
        row.setBrowseIndex(index);
        row.setRowDisabled(disabled);
    };

    configureRow(browserCaptureCollection,
                 captureCollectionDirs.empty() ? juce::StringArray("No Capture Collections")
                                               : collectionNames(captureCollectionDirs, capturesRoot),
                 captureCollectionIndex,
                 captureCollectionDirs.empty(),
                 false);

    configureRow(browserCaptureFile,
                 captureFiles.empty() ? juce::StringArray("No Capture Files")
                                      : fileNames(captureFiles),
                 captureFileIndex,
                 captureFiles.empty(),
                 !captureFiles.empty());
    browserCaptureFile.setActivateToggleState(captureActive, juce::dontSendNotification);

    const bool irLocked = loadedCaptureIsFullRig;
    configureRow(browserIrCollection,
                 irLocked ? juce::StringArray("CAB BAKED IN")
                          : (irCollectionDirs.empty() ? juce::StringArray("No IR Collections")
                                                      : collectionNames(irCollectionDirs, irRoot)),
                 irLocked ? 0 : irCollectionIndex,
                 irLocked || irCollectionDirs.empty(),
                 false);

    configureRow(browserIrFile,
                 irLocked ? juce::StringArray("CAB BAKED IN")
                          : (irFiles.empty() ? juce::StringArray("No IR Files")
                                             : fileNames(irFiles)),
                 irLocked ? 0 : irFileIndex,
                 irLocked || irFiles.empty(),
                 !irLocked && !irFiles.empty());
    browserIrFile.setActivateToggleState(!irLocked && irActive, juce::dontSendNotification);

    resized();
}

void NamUiEditor::resized()
{
    constexpr int innerPad = 16;
    const int ioW = juce::jmax(80, (getWidth() - 2 * innerPad - kIoGap) / 2);
    inputSource.setBounds(innerPad, kIoStripTop, ioW, kIoStripHeight);
    outputSource.setBounds(innerPad + ioW + kIoGap, kIoStripTop, ioW, kIoStripHeight);

    presetPanel.setBounds(innerPad, kPresetTop, getWidth() - 2 * innerPad, NamUiPresetPanel::kDefaultHeight);

    const int clusterW = getWidth() - 2 * kBrowserPadX;
    int y = kBrowserTop;

    for (auto* row : {static_cast<NamUiBrowserRow*>(&browserCaptureCollection),
                      &browserCaptureFile, &browserIrCollection, &browserIrFile})
    {
        row->setBounds(kBrowserPadX, y, clusterW, NamUiBrowserRow::kDefaultHeight);
        y += NamUiBrowserRow::kDefaultHeight + kRowGap;
    }

    mainControlsStrip.setBounds(innerPad, kMainStripTop, getWidth() - 2 * innerPad, kMainStripHeight);
}

} // namespace NamUi
