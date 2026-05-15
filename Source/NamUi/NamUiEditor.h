#pragma once

#include "NamUiBrowserRow.h"
#include "NamUiColourPalette.h"
#include "NamUiMainControlsStrip.h"
#include "NamUiMetronomeStrip.h"
#include "NamUiPresetPanel.h"
#include "NamUiSourceSelector.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

class NamJUCEAudioProcessor;

namespace NamUi
{

/** Root plugin UI — clean slate for the mockUi port (see mockUi/index.jsx). */
class NamUiEditor : public juce::Component,
                    private juce::Timer
{
public:
    explicit NamUiEditor(NamJUCEAudioProcessor& processor);
    ~NamUiEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseDown(const juce::MouseEvent& e) override;

    /// Concept size from mock (~380×720); adjust when layout settles.
    static constexpr int kDesignWidth = 380;
    static constexpr int kDesignHeight = 748;

    /** Shared semantic colours (dark theme); swap struct when light mode lands. */
    const ColourTokens& getColourTokens() const noexcept { return colours; }
    bool handlePresetArrowKey(const juce::KeyPress& key);

    struct PresetEntry
    {
        juce::String id;
        juce::String name;
    };

    struct PresetSnapshot
    {
        juce::String id;
        juce::String name;
        juce::String capturePath;
        juce::String irPath;
        juce::String irCollection;
        NamUiMainControlsStrip::ControlState controls;
        bool captureActive { false };
        bool irActive { false };
    };

private:
    /** Mock root typography: popup menus + lone prototype TextButton use embedded faces from index.jsx. */
    struct EditorLookAndFeel : juce::LookAndFeel_V4
    {
        juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
        juce::Font getPopupMenuFont() override;
    };

    void refreshNamDirectoryFromSettings();
    void refreshBrowserRows();
    void timerCallback() override;
    void refreshPresetLibrary();
    void syncPresetPanel();
    void markPresetDirty();
    void loadPresetAt(int index, int direction);
    void writeCurrentPreset();
    void createPreset(juce::String name);
    void renameCurrentPreset(juce::String name);
    void deleteCurrentPreset();
    void movePreset(int from, int to);
    PresetSnapshot makeCurrentPresetSnapshot(juce::String id, juce::String name) const;
    void applyPresetSnapshot(const PresetSnapshot& snapshot);
    NamUiMainControlsStrip::ControlState makeMainControlStateFromProcessor() const;
    void applyMainControlStateToProcessor(const NamUiMainControlsStrip::ControlState& state);

    NamJUCEAudioProcessor& audioProcessor;
    ColourTokens colours;

    EditorLookAndFeel editorLookAndFeel;

    NamUiSourceSelector inputSource { colours, NamUiSourceSelector::Role::input };
    NamUiSourceSelector outputSource { colours, NamUiSourceSelector::Role::output };

    NamUiPresetPanel presetPanel { colours };

    NamUiMainControlsStrip mainControlsStrip { colours };
    NamUiMetronomeStrip metronomeStrip { colours, audioProcessor.apvts };

    NamUiBrowserRow browserCaptureCollection { colours };
    NamUiBrowserRow browserCaptureFile { colours };
    NamUiBrowserRow browserIrCollection { colours };
    NamUiBrowserRow browserIrFile { colours };

    juce::File namRootDirectory;
    juce::String lastObservedNamDirectoryPath;
    bool namDirectoryStateInitialised { false };
    std::vector<juce::File> captureCollectionDirs;
    std::vector<juce::File> captureFiles;
    std::vector<juce::File> irCollectionDirs;
    std::vector<juce::File> irFiles;
    int captureCollectionIndex { 0 };
    int captureFileIndex { 0 };
    int irCollectionIndex { 0 };
    int irFileIndex { 0 };
    bool captureLoaded { false };
    bool captureActive { false };
    bool irLoaded { false };
    bool irActive { false };
    bool loadedCaptureIsFullRig { false };
    int loadedCaptureCollectionIndex { -1 };
    int loadedCaptureFileIndex { -1 };

    std::vector<PresetEntry> presetEntries;
    PresetSnapshot loadedPresetSnapshot;
    bool hasLoadedPresetSnapshot { false };
    bool presetDirty { false };
    int presetIndex { 0 };
    bool applyingPreset { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiEditor)
};

} // namespace NamUi
