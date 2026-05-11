#pragma once

#include <juce_graphics/juce_graphics.h>

namespace NamUi::Fonts
{

/** Loads Michroma, Share Tech Mono, and Rajdhani from BinaryData on first use. */
void ensureLoaded();

/** Shared letter spacing for Michroma chrome: preset name, buttons, headers, and caps labels. */
inline constexpr float michromaKerningFactorDefault = -0.04f;

/** Primary UI / preset strip / caps labels — matches mock `Michroma`. */
juce::Font michroma(float heightPt, float extraKerningFactor = michromaKerningFactorDefault);

/** Readouts, counters, browser rows, I/O values — matches mock `Share Tech Mono`. */
juce::Font shareTechMono(float heightPt, float extraKerningFactor = 0.f);

/** Root chrome / future body copy — matches mock `Rajdhani`. */
juce::Font rajdhani(float heightPt, float extraKerningFactor = 0.f);

/** Default Michroma height (pt) for the caption under `NamUiKnob` dials. */
inline constexpr float knobLabelHeightPtDefault = 10.f;

/** Current knob-underlabel size; other chrome labels keep their own sizes in components. */
float getKnobLabelHeightPt() noexcept;

/** Clamp ~6…32 pt. After changing, resize or `resized()` parents so `getRecommendedWidth/Height` apply. */
void setKnobLabelHeightPt(float heightPt);

} // namespace NamUi::Fonts
