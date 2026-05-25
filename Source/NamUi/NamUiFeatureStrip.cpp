#include "NamUiFeatureStrip.h"

namespace NamUi
{

NamUiFeatureStrip::NamUiFeatureStrip(const ColourTokens& palette, juce::AudioProcessorValueTreeState& apvts)
    : tokens(palette),
      metronomeStrip(palette, apvts)
{
    setOpaque(true);
    addAndMakeVisible(metronomeStrip);
}

void NamUiFeatureStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);

    g.setColour(tokens.bgPanel);
    g.fillRoundedRectangle(bounds, 2.f);
    g.setColour(tokens.border);
    g.drawRoundedRectangle(bounds, 2.f, 1.f);
}

void NamUiFeatureStrip::resized()
{
    auto area = getLocalBounds().reduced(kInnerPad);

    metronomeStrip.setBounds(area.removeFromLeft(NamUiMetronomeStrip::kDefaultWidth));
    juce::ignoreUnused(area.removeFromLeft(kFeatureItemGap));
}

} // namespace NamUi
