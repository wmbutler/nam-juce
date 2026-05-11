#pragma once

#include <juce_graphics/juce_graphics.h>

namespace NamUi
{

/**
 * Semantic palette from mockUi/index.jsx `C` — dark theme (default).
 * Light theme: add a second token set later and swap at runtime (see mock header ~127–129).
 */
struct ColourTokens
{
    juce::Colour bg          { 0xff0e0e10 }; // Plugin background
    juce::Colour bgDeep      { 0xff080809 }; // Reserved — deeper panels
    juce::Colour bgPanel     { 0xff111114 }; // Panel / section backgrounds
    juce::Colour bgDisplay   { 0xff06060a }; // LCD preset display
    juce::Colour border      { 0xff2a2a32 }; // Standard border
    juce::Colour borderDim   { 0xff1a1a20 }; // Subtle / inactive border
    juce::Colour amber       { 0xfff5a623 }; // Primary amber (knobs, display accents)
    /// Preset strip / nav / mute accent — mock `#f58c00` (distinct from `amber`)
    juce::Colour accentOrange { 0xfff58c00 };
    juce::Colour amberDim    { 0xff7a4f0a }; // Dimmed amber
    juce::Colour amberGlow   { juce::Colour::fromFloatRGBA(245.f / 255.f, 166.f / 255.f, 35.f / 255.f, 0.15f) };
    juce::Colour amberGlow2  { juce::Colour::fromFloatRGBA(245.f / 255.f, 166.f / 255.f, 35.f / 255.f, 0.06f) };
    juce::Colour green       { 0xff39e97b }; // Active / enabled indicator
    juce::Colour greenGlow   { juce::Colour::fromFloatRGBA(57.f / 255.f, 233.f / 255.f, 123.f / 255.f, 0.35f) };
    juce::Colour greenDark   { 0xff1a3d2a }; // Reserved
    juce::Colour red         { 0xffe94040 }; // Danger / armed (DELETE, OVERWRITE)
    juce::Colour textPrimary { 0xffe8e8f0 }; // Primary text, knob labels
    juce::Colour textDim     { 0xff555566 }; // Inactive / placeholder
    juce::Colour textMid     { 0xff8888aa }; // Secondary labels, meter labels
    juce::Colour meterGreen  { 0xff2ecc71 }; // Meter safe zone
    juce::Colour meterYellow { 0xfff1c40f }; // Meter warning (>65%)
    juce::Colour meterRed    { 0xffe74c3c }; // Meter clip (>85%)
    /// LED (`NamUiLed`) inactive fill / border — mock inline hex on `LED` component
    juce::Colour ledInactiveFill   { 0xff1e1e28 };
    juce::Colour ledInactiveBorder { 0xff333344 };
};

} // namespace NamUi
