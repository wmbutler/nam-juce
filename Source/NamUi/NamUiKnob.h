#pragma once

#include "NamUiColourPalette.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

namespace NamUi
{

/** Matches mockUi `Knob` readout formatting (Share Tech Mono strip above knob). */
enum class NamUiKnobReadoutKind
{
    genericPercent,
    inputOutputLevelDb, ///< ±20 dB, unity at 0.5 normalized
    toneEqDb,           ///< ±12 dB, flat at 0.5
    gateThresholdDb     ///< -100…0 dB from 0…1
};

/**
 * Mock `Knob`: normalized 0…1, vertical drag (~150 px / full span), double-click resets.
 * Arc indicator −135° → +135°, amber dot, radial body gradient.
 * Value readout and parameter label sit in separate bordered strips (mock Share Tech Mono / Michroma).
 * Label strip height follows `NamUi::Fonts::getKnobLabelHeightPt()` / `setKnobLabelHeightPt`.
 */
class NamUiKnob : public juce::Component
{
public:
    NamUiKnob(const ColourTokens& palette, NamUiKnobReadoutKind readoutKind, const juce::String& knobLabel,
              int knobDiameterPixels = 40, float defaultNormalized = 0.5f);

    void setReadoutKind(NamUiKnobReadoutKind kind);
    void setLabel(const juce::String& knobLabel);
    void setKnobDiameter(int diameterPixels);
    void setDefaultNormalized(float value);
    void setValueNormalized(double value, juce::NotificationType notify = juce::sendNotification);

    double getValueNormalized() const noexcept { return valueNormalized; }
    float getDefaultNormalized() const noexcept { return defaultNormalized; }
    int getKnobDiameter() const noexcept { return knobDiameterPixels; }

    /** Total bounds height for layout (readout + gaps + knob + label). */
    int getRecommendedHeight() const;
    int getRecommendedWidth() const;

    /** Width like `getRecommendedWidth()` but readout strip is sized for `sampleReadout` (e.g. `"+20.0dB"`). */
    int getRecommendedWidthEnsuringReadout(const juce::String& sampleReadout) const;

    /** Width for FlexBox layout using longest typical readout per kind (avoids clipping / overflow vs current value). */
    int getRecommendedWidthForLayout() const;

    std::function<void(double)> onValueChange;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    juce::String formatReadout() const;
    void paintReadoutStrip(juce::Graphics& g, juce::Rectangle<int> area) const;
    void paintLabelStrip(juce::Graphics& g, juce::Rectangle<int> area) const;

    const ColourTokens& tokens;
    NamUiKnobReadoutKind readoutKind { NamUiKnobReadoutKind::genericPercent };
    juce::String label;
    int knobDiameterPixels { 40 };
    float defaultNormalized { 0.5f };
    double valueNormalized { 0.5 };

    int dragStartY { 0 };
    double dragStartValue { 0. };

    static constexpr float kPixelsPerFullSweep = 150.f;
    static constexpr int kSectionGap = 3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiKnob)
};

} // namespace NamUi
