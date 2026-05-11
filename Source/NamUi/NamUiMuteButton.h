#pragma once

#include "NamUiColourPalette.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace NamUi
{

/**
 * Mock `MuteButton`: orange outline, lit dot when muted (toggle).
 * ToggleState true == muted == mock `active`.
 */
class NamUiMuteButton : public juce::ToggleButton
{
public:
    explicit NamUiMuteButton(const ColourTokens& palette);

    /** Width / height that match `paintButton` padding, dot, and label — use for layout instead of a fixed width. */
    int getIdealWidth() const;
    int getIdealHeight() const;

protected:
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    const ColourTokens& tokens;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiMuteButton)
};

} // namespace NamUi
