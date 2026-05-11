#pragma once

#include "NamUiColourPalette.h"
#include "NamUiStandardButton.h"
#include <functional>

namespace NamUi
{

/**
 * Mock `PresetDisplay`: LCD panel, preset name, PREV / counter / NEXT, vertical divider,
 * soft column (NEW / SAVE / RENAME / DELETE), manifest shift buttons, naming mode + WRITTEN flash.
 */
class NamUiPresetPanel : public juce::Component, private juce::TextEditor::Listener
{
public:
    static constexpr int kDefaultHeight = 134;

    explicit NamUiPresetPanel(const ColourTokens& palette);
    ~NamUiPresetPanel() override;

    void setPresetName(juce::String name);
    const juce::String& getPresetName() const noexcept { return presetName; }

    /** Zero-based index and total count; counter shows "—" when total is 0. */
    void setCounter(int indexZeroBased, int total);

    /** Enables SAVE / RENAME / DELETE / shift when a preset is considered loaded. */
    void setHasLoadedPreset(bool loaded);
    bool hasLoadedPreset() const noexcept { return presetLoadedFlag; }

    /** Dirty state drives NEW (with total>0) and SAVE enablement (mock semantics). */
    void setPresetDirty(bool dirty);
    bool isPresetDirty() const noexcept { return presetDirtyFlag; }
    bool isNamingActive() const noexcept { return namingMode; }

    void setOnPrev(std::function<void()> callback);
    void setOnNext(std::function<void()> callback);

    void setOnSave(std::function<void()> callback);
    void setOnNewPreset(std::function<void(juce::String name)> callback);
    void setOnRenamePreset(std::function<void(juce::String newName)> callback);
    void setOnDeletePreset(std::function<void()> callback);
    /** Reorder: move preset at `fromIndex` to `toIndex` (indices before move). */
    void setOnMovePreset(std::function<void(int fromIndex, int toIndex)> callback);

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    void textEditorTextChanged(juce::TextEditor& editor) override;

    void updateNavEnabled();
    void updateSoftButtonStates();
    void syncSaveDeleteButtonLabels();

    void enterNamingMode(bool forNewPreset);
    void exitNamingMode();
    void commitNamingSave();

    void handleSoftNew();
    void handleSoftSaveClick();
    void handleSoftRename();
    void handleSoftDeleteClick();
    void handleShiftLeft();
    void handleShiftRight();

    void armSaveSecondStep();
    void clearSaveArm();
    void armDeleteSecondStep();
    void clearDeleteArm();

    void showWrittenFlash();

    const ColourTokens& tokens;
    juce::String presetName { "EMPTY PRESET" };
    int presetIndex { 0 };
    int presetTotal { 0 };

    bool presetDirtyFlag { false };
    bool presetLoadedFlag { false };

    bool namingMode { false };
    bool namingIsNew { false };

    bool saveArmed { false };
    bool deleteArmed { false };
    uint32_t saveArmGeneration { 0 };
    uint32_t deleteArmGeneration { 0 };

    juce::String flashText;
    uint32_t flashGeneration { 0 };

    juce::Rectangle<int> mainColumnBounds;
    juce::Rectangle<int> softColumnBounds;

    juce::Rectangle<int> nameBounds;
    juce::Rectangle<int> counterBounds;

    NamUiStandardButton presetPrev { tokens, StandardButtonStyle::presetNav, "PREV" };
    NamUiStandardButton presetNext { tokens, StandardButtonStyle::presetNav, "NEXT" };

    NamUiStandardButton btnNew { tokens, StandardButtonStyle::softPreset, "NEW" };
    NamUiStandardButton btnSave { tokens, StandardButtonStyle::softPreset, "SAVE" };
    NamUiStandardButton btnRename { tokens, StandardButtonStyle::softPreset, "RENAME" };
    NamUiStandardButton btnDelete { tokens, StandardButtonStyle::softPreset, "DELETE" };
    NamUiStandardButton shiftLeft { tokens, StandardButtonStyle::shiftNav, {} };
    NamUiStandardButton shiftRight { tokens, StandardButtonStyle::shiftNav, {} };

    NamUiStandardButton namingCancel { tokens, StandardButtonStyle::presetNav, "CANCEL" };
    NamUiStandardButton namingSave { tokens, StandardButtonStyle::presetNav, "SAVE" };

    juce::TextEditor namingEditor;

    std::function<void()> onPrevCb;
    std::function<void()> onNextCb;
    std::function<void()> onSaveCb;
    std::function<void(juce::String)> onNewPresetCb;
    std::function<void(juce::String)> onRenamePresetCb;
    std::function<void()> onDeletePresetCb;
    std::function<void(int, int)> onMovePresetCb;

    /** Right column fills toward the border; divider stays just left of the soft buttons. */
    static constexpr int kSoftColumnWidth = 82;
    static constexpr int kDividerGap = 6;
    static constexpr int kDividerMarginY = 10;
    /** PREV/NEXT/NEW/SAVE/…/shift row — single height to match mock. */
    static constexpr int kPresetPanelBtnH = 21;
    static constexpr int kNavRowH = kPresetPanelBtnH;
    static constexpr int kPadX = 6;
    static constexpr int kPadRight = 4;
    /** Trimmed so vertical budget feeds `kSoftGap` between right-column buttons. */
    static constexpr int kPadY = 5;
    static constexpr int kSoftGap = 3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiPresetPanel)
};

} // namespace NamUi
