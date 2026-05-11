#pragma once

#include "NamUiColourPalette.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace NamUi
{

/**
 * Unified outline / invert-on-press buttons from mockUi:
 * SoftButton, PresetNavButton, ShiftButton, NamingSaveButton, NavButton (◀ ▶), ActiveButton (CAPTURE / IR).
 * Fonts: embedded Michroma / Share Tech Mono (`NamUiFonts` + BinaryData).
 */
enum class StandardButtonStyle
{
    softPreset, ///< SAVE / RENAME / … — Michroma, radius 2, optional danger (red) + compact size
    presetNav,  ///< PREV / NEXT / SAVE (naming) — Michroma, radius 1
    shiftNav,   ///< ◁ ▷ manifest — Share Tech Mono, radius 1, horizontal stretch in layout
    browserNav, ///< Browser row ◀ ▶ — vertical border separators, fill when pressed
    /** CAPTURE / IR (CAB) — toggle (`setClickingTogglesState`), green + LED; lay out with width ≥ kBrowserActivateMinWidth */
    browserActivate,
};

class NamUiStandardButton : public juce::Button
{
public:
    /// Mock ActiveButton `minWidth` — parent `setBounds` should honour at least this width.
    static constexpr int kBrowserActivateMinWidth = 88;

    NamUiStandardButton(const ColourTokens& palette, StandardButtonStyle buttonStyle, const juce::String& buttonText = {});

    void setDanger(bool shouldBeDanger);
    bool isDanger() const noexcept { return danger; }

    /** SoftPreset only: small=true matches mock `small` (10px text vs 12px). */
    void setSoftCompact(bool compact);
    bool isSoftCompact() const noexcept { return softCompact; }

protected:
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    const ColourTokens& tokens;
    StandardButtonStyle style;
    bool danger { false };
    bool softCompact { false };

    void paintSoftLike(juce::Graphics& g, juce::Rectangle<float> bounds, bool enabled, bool down, float cornerRadius,
                       const juce::Font& font, float padX, float padY, juce::Colour accent);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiStandardButton)
};

} // namespace NamUi
