#pragma once

#include "NamUiColourPalette.h"
#include "NamUiStandardButton.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace NamUi
{

/**
 * Mock `BrowserRow`: prev/next browser nav, optional CAPTURE / IR (CAB) toggle, optional right slot.
 * Does not scan directories — parent supplies `items` / indices and callbacks.
 */
class NamUiBrowserRow : public juce::Component
{
public:
    explicit NamUiBrowserRow(const ColourTokens& palette);

    static constexpr int kDefaultHeight = 36;

    void setRowDisabled(bool disabled);
    bool isRowDisabled() const noexcept { return rowDisabled; }

    void setItems(juce::StringArray lines);
    const juce::StringArray& getItems() const noexcept { return items; }

    /** Clamped to item count. */
    void setBrowseIndex(int idx);
    int getBrowseIndex() const noexcept { return itemIndex; }

    /** Counter denominator; 0 means use `items.size()`. */
    void setBrowseTotal(int totalOrZeroForItemsSize);
    int getBrowseTotal() const noexcept { return effectiveTotal(); }

    /** When false, CAPTURE / IR control is hidden (collection rows, or IR row when full rig). */
    void setShowActivateControl(bool show);
    bool showActivateControl() const noexcept { return showActivate; }

    void setActivateButtonText(const juce::String& label);

    void setActivateToggleState(bool isActive, juce::NotificationType = juce::dontSendNotification);

    void setOnPrev(std::function<void()> fn) { onPrevCb = std::move(fn); }
    void setOnNext(std::function<void()> fn) { onNextCb = std::move(fn); }
    void setOnActivate(std::function<void()> fn);

    /** Non-owning; row adds as child if non-null. Remove by passing nullptr. */
    void setRightAccessory(juce::Component* component);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void refreshContents();
    void wireButtons();
    int effectiveTotal() const;

    const ColourTokens& tokens;

    bool rowDisabled { false };
    bool showActivate { false };

    juce::StringArray items;
    int itemIndex { 0 };
    int totalOverride { 0 };

    juce::String activateLabel;

    NamUiStandardButton btnPrev;
    NamUiStandardButton btnNext;
    NamUiStandardButton btnActivate;

    juce::Label titleLabel;
    juce::Label counterLabel;

    juce::Component* rightAccessory { nullptr };

    std::function<void()> onPrevCb;
    std::function<void()> onNextCb;
    std::function<void()> onActivateCb;

    static int navButtonWidth();
    int resolveCounterWidth() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiBrowserRow)
};

} // namespace NamUi
