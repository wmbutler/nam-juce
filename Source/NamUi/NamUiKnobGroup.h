#pragma once

#include "NamUiColourPalette.h"
#include "NamUiLed.h"
#include <functional>

namespace NamUi
{

/**
 * Mock `KnobGroup`: bordered panel, header row (LED + title) toggles active state;
 * inactive knobs are disabled and drawn faint (see `NamUiKnob`).
 */
class NamUiKnobGroup : public juce::Component
{
public:
    explicit NamUiKnobGroup(const ColourTokens& palette, juce::String groupTitle);

    void setActive(bool shouldBeActive);
    bool isActive() const noexcept { return active; }

    std::function<void(bool)> onActiveChange;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /** Horizontal gap between knobs in this row (first knob has no leading gap). Default matches mock ~16px. */
    void setInterKnobGapPixels(float gapPx);

private:
    class Header : public juce::Component
    {
    public:
        Header(NamUiKnobGroup& ownerIn, const ColourTokens& paletteIn, juce::String titleIn);

        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseEnter(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;

    private:
        NamUiKnobGroup& owner;
        const ColourTokens& tokens;
        NamUiLed led;
        juce::String title;
        bool hovered { false };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Header)
    };

    void syncChildrenEnabled();
    void toggleActive();

    const ColourTokens& tokens;
    Header header;
    bool active { true };
    float interKnobGapPx { 16.f };

    static constexpr int kHeaderH = 28;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiKnobGroup)
};

} // namespace NamUi
