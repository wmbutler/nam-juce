#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NamJUCEAudioProcessorEditor::NamJUCEAudioProcessorEditor(NamJUCEAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), namUiEditor(p)
{
    setWantsKeyboardFocus(true);
    setResizable(false, false);
    setSize(NamUi::NamUiEditor::kDesignWidth, NamUi::NamUiEditor::kDesignHeight);

    addAndMakeVisible(namUiEditor);
    namUiEditor.setBounds(getLocalBounds());
    grabKeyboardFocus();
}

NamJUCEAudioProcessorEditor::~NamJUCEAudioProcessorEditor() = default;

//==============================================================================
void NamJUCEAudioProcessorEditor::paint(juce::Graphics&) {}

void NamJUCEAudioProcessorEditor::resized()
{
    namUiEditor.setBounds(getLocalBounds());
}

bool NamJUCEAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    return namUiEditor.handlePresetArrowKey(key);
}
