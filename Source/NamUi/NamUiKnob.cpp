#include "NamUiKnob.h"
#include "NamUiFonts.h"
#include <cmath>

namespace NamUi
{

namespace
{
static constexpr int kReadoutPadX = 6;
static constexpr int kReadoutPadY = 3;
static constexpr int kLabelPadX = 6;
static constexpr int kLabelPadY = 4;
/// Minimum outer height for the value strip (mock minHeight 12 + padding).
static constexpr int kReadoutStripMinOuterH = 16;

juce::Font makeKnobReadoutFontBase()
{
    return Fonts::shareTechMono(12.f, 0.f);
}

/** Letter-spaced body only — never apply kerning to the unit (`dB`, `%`) or a gap appears before it. */
juce::Font makeKnobReadoutFontKerned()
{
    return Fonts::shareTechMono(12.f, 0.1f);
}

void splitReadoutMainAndUnit(const juce::String& s, juce::String& mainPart, juce::String& unitSuffix)
{
    if (s.endsWith("dB"))
    {
        mainPart = s.substring(0, s.length() - 2);
        unitSuffix = "dB";
    }
    else if (s.isNotEmpty() && s[s.length() - 1] == '%')
    {
        mainPart = s.substring(0, s.length() - 1);
        unitSuffix = "%";
    }
    else
    {
        mainPart = s;
        unitSuffix = {};
    }
}

float measureReadoutWidth(const juce::String& fullReadout)
{
    juce::String mainBit, unitBit;
    splitReadoutMainAndUnit(fullReadout, mainBit, unitBit);
    return makeKnobReadoutFontKerned().getStringWidthFloat(mainBit) + makeKnobReadoutFontBase().getStringWidthFloat(unitBit);
}

juce::Font makeKnobLabelFont()
{
    return Fonts::michroma(Fonts::getKnobLabelHeightPt());
}

int readoutStripOuterHeight()
{
    const float innerTextH = makeKnobReadoutFontBase().getHeight();
    const int h = juce::roundToInt(innerTextH + (float)kReadoutPadY * 2.f);
    return juce::jmax(kReadoutStripMinOuterH, h);
}

int labelStripOuterHeight(const juce::String& knobLabel)
{
    const auto lines = juce::jmax(1, juce::StringArray::fromLines(knobLabel).size());
    const auto f = makeKnobLabelFont();
    const float lineH = f.getHeight() * 1.5f;
    return kLabelPadY * 2 + juce::roundToInt(lineH * (float)lines);
}
} // namespace

NamUiKnob::NamUiKnob(const ColourTokens& palette, NamUiKnobReadoutKind kind, const juce::String& knobLabel,
                     int knobDiameter, float defaultNorm)
    : tokens(palette), readoutKind(kind), label(knobLabel), knobDiameterPixels(knobDiameter),
      defaultNormalized(defaultNorm), valueNormalized(defaultNorm)
{
    setInterceptsMouseClicks(true, false);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void NamUiKnob::setReadoutKind(NamUiKnobReadoutKind kind)
{
    readoutKind = kind;
    repaint();
}

void NamUiKnob::setLabel(const juce::String& knobLabel)
{
    label = knobLabel;
    repaint();
}

void NamUiKnob::setKnobDiameter(int diameterPixels)
{
    knobDiameterPixels = juce::jmax(24, diameterPixels);
    repaint();
}

void NamUiKnob::setDefaultNormalized(float value)
{
    defaultNormalized = juce::jlimit(0.f, 1.f, value);
}

void NamUiKnob::setValueNormalized(double value, juce::NotificationType notify)
{
    value = juce::jlimit(0.0, 1.0, value);
    if (std::abs(value - valueNormalized) < 1.0e-9)
        return;
    valueNormalized = value;
    repaint();
    if (notify != juce::dontSendNotification && onValueChange)
        onValueChange(valueNormalized);
}

juce::String NamUiKnob::formatReadout() const
{
    switch (readoutKind)
    {
    case NamUiKnobReadoutKind::gateThresholdDb:
    {
        const double db = -100.0 + valueNormalized * 100.0;
        if (db >= 0.0)
            return juce::String("0.0dB");
        return juce::String(db, 1) + "dB";
    }
    case NamUiKnobReadoutKind::inputOutputLevelDb:
    {
        const double db = (valueNormalized - 0.5) * 40.0;
        return juce::String(db >= 0.0 ? "+" : "") + juce::String(db, 1) + "dB";
    }
    case NamUiKnobReadoutKind::toneEqDb:
    {
        const double db = (valueNormalized - 0.5) * 24.0;
        return juce::String(db >= 0.0 ? "+" : "") + juce::String(db, 1) + "dB";
    }
    case NamUiKnobReadoutKind::genericPercent:
    default:
        return juce::String(juce::roundToInt(valueNormalized * 100.0)) + "%";
    }
}

int NamUiKnob::getRecommendedHeight() const
{
    return readoutStripOuterHeight() + kSectionGap + knobDiameterPixels + kSectionGap + labelStripOuterHeight(label);
}

int NamUiKnob::getRecommendedWidth() const
{
    return getRecommendedWidthEnsuringReadout(formatReadout());
}

int NamUiKnob::getRecommendedWidthEnsuringReadout(const juce::String& sampleReadout) const
{
    const auto labelFont = makeKnobLabelFont();
    const int borderInset = 2;
    const auto readoutW =
        juce::roundToInt(measureReadoutWidth(sampleReadout)) + kReadoutPadX * 2 + borderInset;

    int labelMax = 0;
    for (auto line : juce::StringArray::fromLines(label))
        labelMax = juce::jmax(labelMax, juce::roundToInt(labelFont.getStringWidthFloat(line)));

    const auto labelW = labelMax + kLabelPadX * 2 + borderInset;
    return juce::jmax(knobDiameterPixels + 4, readoutW, labelW);
}

int NamUiKnob::getRecommendedWidthForLayout() const
{
    switch (readoutKind)
    {
    case NamUiKnobReadoutKind::toneEqDb:
        return getRecommendedWidthEnsuringReadout("+12.0dB");
    case NamUiKnobReadoutKind::inputOutputLevelDb:
        return getRecommendedWidthEnsuringReadout("+20.0dB");
    case NamUiKnobReadoutKind::gateThresholdDb:
        return getRecommendedWidthEnsuringReadout("-100.0dB");
    case NamUiKnobReadoutKind::genericPercent:
    default:
        return getRecommendedWidth();
    }
}

void NamUiKnob::paintReadoutStrip(juce::Graphics& g, juce::Rectangle<int> area) const
{
    const bool controlEnabled = isEnabled();

    auto rf = area.toFloat().reduced(0.5f);
    g.setColour(tokens.bgDisplay);
    g.fillRoundedRectangle(rf, 2.f);
    g.setColour(tokens.borderDim);
    g.drawRoundedRectangle(rf, 2.f, 1.f);

    auto inner = area.reduced(kReadoutPadX, kReadoutPadY);
    g.setColour(controlEnabled ? tokens.textPrimary : tokens.textDim);

    const auto full = formatReadout();
    juce::String mainBit, unitBit;
    splitReadoutMainAndUnit(full, mainBit, unitBit);

    const auto fk = makeKnobReadoutFontKerned();
    const auto fb = makeKnobReadoutFontBase();
    const float wMain = fk.getStringWidthFloat(mainBit);
    const float wUnit = fb.getStringWidthFloat(unitBit);
    const float totalW = wMain + wUnit;

    auto innerF = inner.toFloat();
    const float startX = innerF.getCentreX() - totalW * 0.5f;

    juce::Rectangle<float> rMain(startX, innerF.getY(), wMain, innerF.getHeight());
    juce::Rectangle<float> rUnit(startX + wMain, innerF.getY(), wUnit, innerF.getHeight());

    g.setFont(fk);
    g.drawText(mainBit, rMain, juce::Justification::centredLeft, false);
    if (unitBit.isNotEmpty())
    {
        g.setFont(fb);
        g.drawText(unitBit, rUnit, juce::Justification::centredLeft, false);
    }
}

void NamUiKnob::paintLabelStrip(juce::Graphics& g, juce::Rectangle<int> area) const
{
    const bool controlEnabled = isEnabled();

    auto rf = area.toFloat().reduced(0.5f);
    g.setColour(tokens.bgPanel);
    g.fillRoundedRectangle(rf, 2.f);
    g.setColour(tokens.borderDim);
    g.drawRoundedRectangle(rf, 2.f, 1.f);

    auto inner = area.reduced(kLabelPadX, kLabelPadY);
    auto font = makeKnobLabelFont();
    g.setFont(font);
    g.setColour(controlEnabled ? tokens.textPrimary : tokens.textDim);

    const auto lines = juce::StringArray::fromLines(label);
    const float lineH = font.getHeight() * 1.5f;
    const float totalH = lineH * (float)juce::jmax(1, lines.size());
    float y = (float)inner.getCentreY() - totalH * 0.5f;

    for (int i = 0; i < lines.size(); ++i)
    {
        auto lineRect = inner.withY(juce::roundToInt(y)).withHeight(juce::roundToInt(lineH));
        g.drawText(lines[i], lineRect, juce::Justification::centred, false);
        y += lineH;
    }
}

void NamUiKnob::paint(juce::Graphics& g)
{
    const bool controlEnabled = isEnabled();

    auto bounds = getLocalBounds();
    auto readoutArea = bounds.removeFromTop(readoutStripOuterHeight());
    bounds.removeFromTop(kSectionGap);
    auto knobArea = bounds.removeFromTop(knobDiameterPixels);
    bounds.removeFromTop(kSectionGap);
    auto labelArea = bounds;

    paintReadoutStrip(g, readoutArea);

    const float size = (float)knobDiameterPixels;
    const float cx = knobArea.getCentreX();
    const float cy = knobArea.getCentreY();
    const float rTrack = size * 0.5f - 6.f;
    const float rBody = size * 0.5f - 3.f;

    {
        juce::Graphics::ScopedSaveState bodyScope(g);
        if (!controlEnabled)
            g.setOpacity(0.35f);

        g.setColour(tokens.borderDim);
        g.drawEllipse(cx - rTrack, cy - rTrack, rTrack * 2.f, rTrack * 2.f, 3.f);

        juce::ColourGradient bodyGrad(juce::Colour(0xff2e2e3e), cx - rBody * 0.2f, cy - rBody * 0.35f,
                                      juce::Colour(0xff141420), cx + rBody * 0.9f, cy + rBody * 0.9f, true);
        g.setGradientFill(bodyGrad);
        g.fillEllipse(cx - rBody, cy - rBody, rBody * 2.f, rBody * 2.f);

        g.setColour(controlEnabled ? tokens.border : tokens.borderDim);
        g.drawEllipse(cx - rBody, cy - rBody, rBody * 2.f, rBody * 2.f, 1.f);
    }

    const double angleDeg = -135.0 + valueNormalized * 270.0;
    const float rad = juce::degreesToRadians((float)angleDeg - 90.f);
    const float px = cx + rTrack * std::cos(rad);
    const float py = cy + rTrack * std::sin(rad);
    const float dotR = 3.f;

    const auto dotLit = controlEnabled ? tokens.amber : tokens.amberDim;
    g.setColour(dotLit.withAlpha(controlEnabled ? 0.45f : 0.28f));
    g.fillEllipse(px - dotR - 1.5f, py - dotR - 1.5f, (dotR + 1.5f) * 2.f, (dotR + 1.5f) * 2.f);
    g.setColour(dotLit);
    g.fillEllipse(px - dotR, py - dotR, dotR * 2.f, dotR * 2.f);

    paintLabelStrip(g, labelArea);
}

void NamUiKnob::mouseDown(const juce::MouseEvent& e)
{
    if (!isEnabled())
        return;
    dragStartY = e.y;
    dragStartValue = valueNormalized;
}

void NamUiKnob::mouseDrag(const juce::MouseEvent& e)
{
    if (!isEnabled())
        return;
    const float delta = (float)(dragStartY - e.y) / kPixelsPerFullSweep;
    setValueNormalized(juce::jlimit(0.0, 1.0, dragStartValue + (double)delta));
}

void NamUiKnob::mouseDoubleClick(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (!isEnabled())
        return;
    setValueNormalized((double)defaultNormalized);
}

} // namespace NamUi
