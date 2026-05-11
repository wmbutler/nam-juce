#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "NamUi/NamUiEditor.h"

//==============================================================================
class NamJUCEAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    NamJUCEAudioProcessorEditor(NamJUCEAudioProcessor&);
    ~NamJUCEAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    NamUi::NamUiEditor namUiEditor;

    NamJUCEAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamJUCEAudioProcessorEditor)
};
