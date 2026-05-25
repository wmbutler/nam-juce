#pragma once

#include "NamUiColourPalette.h"
#include "NamUiMetronomeStrip.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace NamUi
{

/** Full-width bottom tray for utility features (metronome first; more items later). */
class NamUiFeatureStrip : public juce::Component
{
public:
    static constexpr int kDefaultHeight = 48;

    NamUiFeatureStrip(const ColourTokens& palette, juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;
    void resized() override;

    NamUiMetronomeStrip& getMetronomeStrip() noexcept { return metronomeStrip; }

private:
    const ColourTokens& tokens;
    NamUiMetronomeStrip metronomeStrip;

    static constexpr int kInnerPad = 4;
    static constexpr int kFeatureItemGap = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiFeatureStrip)
};

} // namespace NamUi
