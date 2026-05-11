#pragma once

#include "NamUiColourPalette.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace NamUi
{

/**
 * Group-header indicator from mockUi `LED`: 10×10, lit = radial-style fill + glow.
 * Mouse clicks pass through by default (mock: header button is the hit target).
 */
class NamUiLed : public juce::Component
{
public:
    explicit NamUiLed(const ColourTokens& palette);

    void setActive(bool shouldBeActive);
    bool isActive() const noexcept { return active; }

    /** Default lit uses palette green / greenGlow; override for other LEDs if needed. */
    void setLitColours(juce::Colour lit, juce::Colour glow);

    void paint(juce::Graphics& g) override;

    static constexpr int kDefaultSide = 10;

private:
    const ColourTokens& tokens;
    bool active { false };
    juce::Colour litColour;
    juce::Colour glowColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiLed)
};

} // namespace NamUi
