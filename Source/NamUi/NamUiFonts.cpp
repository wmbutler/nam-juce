#include "NamUiFonts.h"
#include <BinaryData.h>
#include <mutex>

namespace NamUi::Fonts
{
namespace
{
std::once_flag loadFlag;

// Hold embedded faces on the heap and never destroy them. Static Typeface::Ptr globals are torn down
// during exit() after other teardown; on macOS that can abort inside CoreTextTypeface::~CoreTextTypeface().
// Leaking these pointers only affects process exit (OS reclaims the address space).
juce::Typeface::Ptr* leakedMichroma = nullptr;
juce::Typeface::Ptr* leakedShareTech = nullptr;
juce::Typeface::Ptr* leakedRajdhani = nullptr;

void loadEmbeddedFaces()
{
    leakedMichroma = new juce::Typeface::Ptr();
    leakedShareTech = new juce::Typeface::Ptr();
    leakedRajdhani = new juce::Typeface::Ptr();

    if (BinaryData::Michroma_ttfSize > 0)
        *leakedMichroma = juce::Typeface::createSystemTypefaceFor(BinaryData::Michroma_ttf,
                                                                     (size_t)BinaryData::Michroma_ttfSize);

    if (BinaryData::ShareTechMono_ttfSize > 0)
        *leakedShareTech = juce::Typeface::createSystemTypefaceFor(BinaryData::ShareTechMono_ttf,
                                                                     (size_t)BinaryData::ShareTechMono_ttfSize);

    if (BinaryData::Rajdhani_ttfSize > 0)
        *leakedRajdhani = juce::Typeface::createSystemTypefaceFor(BinaryData::Rajdhani_ttf,
                                                                   (size_t)BinaryData::Rajdhani_ttfSize);
}

juce::Font makeFont(juce::Typeface::Ptr face, float heightPt, float kerning, const juce::String& fallbackName)
{
    auto opts = juce::FontOptions().withHeight(heightPt).withMetricsKind(juce::TypefaceMetricsKind::portable);

    if (face != nullptr)
        opts = opts.withTypeface(face);
    else if (fallbackName.isNotEmpty())
        opts = opts.withName(fallbackName);

    juce::Font f(opts);
    f.setExtraKerningFactor(kerning);
    return f;
}
} // namespace

static float gKnobLabelHeightPt = knobLabelHeightPtDefault;

float getKnobLabelHeightPt() noexcept
{
    return gKnobLabelHeightPt;
}

void setKnobLabelHeightPt(float heightPt)
{
    gKnobLabelHeightPt = juce::jlimit(6.f, 32.f, heightPt);
}

void ensureLoaded()
{
    std::call_once(loadFlag, loadEmbeddedFaces);
}

juce::Font michroma(float heightPt, float extraKerningFactor)
{
    ensureLoaded();
    return makeFont(leakedMichroma != nullptr ? *leakedMichroma : nullptr,
                    heightPt,
                    extraKerningFactor,
                    juce::Font::getDefaultSansSerifFontName());
}

juce::Font shareTechMono(float heightPt, float extraKerningFactor)
{
    ensureLoaded();
    return makeFont(leakedShareTech != nullptr ? *leakedShareTech : nullptr,
                    heightPt,
                    extraKerningFactor,
                    juce::Font::getDefaultMonospacedFontName());
}

juce::Font rajdhani(float heightPt, float extraKerningFactor)
{
    ensureLoaded();
    return makeFont(leakedRajdhani != nullptr ? *leakedRajdhani : nullptr,
                    heightPt,
                    extraKerningFactor,
                    juce::Font::getDefaultSansSerifFontName());
}

} // namespace NamUi::Fonts
