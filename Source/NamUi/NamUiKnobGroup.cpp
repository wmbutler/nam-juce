#include "NamUiKnobGroup.h"
#include "NamUiKnob.h"
#include "NamUiFonts.h"

namespace NamUi
{

NamUiKnobGroup::Header::Header(NamUiKnobGroup& ownerIn, const ColourTokens& paletteIn, juce::String titleIn)
    : owner(ownerIn), tokens(paletteIn), led(paletteIn), title(std::move(titleIn))
{
    addAndMakeVisible(led);
    setInterceptsMouseClicks(true, false);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void NamUiKnobGroup::Header::resized()
{
    led.setActive(owner.isActive());
    auto r = getLocalBounds().reduced(6, 2);
    const int ledSide = NamUiLed::kDefaultSide;
    static constexpr int kLedTextGap = 6;

    auto font = Fonts::michroma(11.f);
    const int textW = juce::roundToInt(font.getStringWidthFloat(title.toUpperCase()));
    const int totalW = ledSide + kLedTextGap + textW;
    int x0 = r.getCentreX() - totalW / 2;
    x0 = juce::jlimit(r.getX(), juce::jmax(r.getX(), r.getRight() - totalW), x0);
    led.setBounds(x0, r.getCentreY() - ledSide / 2, ledSide, ledSide);
}

void NamUiKnobGroup::Header::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const bool on = owner.isActive();

    juce::Colour bg = juce::Colours::transparentBlack;
    if (hovered)
        bg = on ? juce::Colour::fromFloatRGBA(57.f / 255.f, 233.f / 255.f, 123.f / 255.f, 0.08f)
                : juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 0.04f);

    g.setColour(bg);
    g.fillRoundedRectangle(bounds, 2.f);

    g.setColour(on ? tokens.border : tokens.borderDim);
    g.drawRoundedRectangle(bounds, 2.f, 1.f);

    g.setColour(on ? tokens.textPrimary : tokens.textMid);
    g.setFont(Fonts::michroma(11.f));
    static constexpr int kGap = 6;
    auto ledR = led.getBounds().toFloat();
    auto textArea =
        juce::Rectangle<float>(ledR.getRight() + kGap, bounds.getY(), bounds.getRight() - ledR.getRight() - kGap,
                               bounds.getHeight());
    g.drawText(title.toUpperCase(), textArea, juce::Justification::centredLeft, false);
}

void NamUiKnobGroup::Header::mouseDown(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    owner.toggleActive();
}

void NamUiKnobGroup::Header::mouseEnter(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    hovered = true;
    repaint();
}

void NamUiKnobGroup::Header::mouseExit(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    hovered = false;
    repaint();
}

NamUiKnobGroup::NamUiKnobGroup(const ColourTokens& palette, juce::String groupTitle)
    : tokens(palette), header(*this, palette, std::move(groupTitle))
{
    addAndMakeVisible(header);
}

void NamUiKnobGroup::setInterKnobGapPixels(float gapPx)
{
    interKnobGapPx = juce::jmax(0.f, gapPx);
    resized();
}

void NamUiKnobGroup::setActive(bool shouldBeActive)
{
    if (active == shouldBeActive)
        return;
    active = shouldBeActive;
    header.resized();
    header.repaint();
    syncChildrenEnabled();
    repaint();
}

void NamUiKnobGroup::toggleActive()
{
    setActive(!active);
    if (onActiveChange)
        onActiveChange(active);
}

void NamUiKnobGroup::syncChildrenEnabled()
{
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        auto* c = getChildComponent(i);
        if (c != &header)
            c->setEnabled(active);
    }
}

void NamUiKnobGroup::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(active ? tokens.border : tokens.borderDim);
    g.drawRoundedRectangle(bounds, 2.f, 1.f);
}

void NamUiKnobGroup::resized()
{
    auto full = getLocalBounds();
    header.setBounds(full.removeFromTop(kHeaderH));

    // Horizontal inset: row is FlexBox-centered; if 3×knob + gaps exceed this width, outer knobs spill past the border.
    static constexpr int kContentInsetX = 12;
    auto content = full.withTrimmedLeft(kContentInsetX).withTrimmedRight(kContentInsetX).withTrimmedTop(20).withTrimmedBottom(12);

    juce::Array<NamUiKnob*> knobs;
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        auto* c = getChildComponent(i);
        if (c != &header)
            if (auto* k = dynamic_cast<NamUiKnob*>(c))
                knobs.add(k);
    }

    const int nKnobs = knobs.size();
    float gapPx = interKnobGapPx;
    if (nKnobs > 1)
    {
        float totalW = 0.f;
        for (auto* k : knobs)
            totalW += (float)k->getRecommendedWidthForLayout();

        const float avail = (float)content.getWidth();
        const int nGaps = nKnobs - 1;
        const float maxGap = (avail - totalW) / (float)nGaps;
        gapPx = juce::jlimit(0.f, interKnobGapPx, maxGap);
    }

    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::center;
    fb.alignItems = juce::FlexBox::AlignItems::flexStart;

    bool firstKnob = true;
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        auto* c = getChildComponent(i);
        if (c == &header)
            continue;

        if (auto* k = dynamic_cast<NamUiKnob*>(c))
        {
            auto fi = juce::FlexItem(*k)
                          .withFlex(0.f)
                          .withMinWidth((float)k->getRecommendedWidthForLayout())
                          .withMinHeight((float)k->getRecommendedHeight());
            if (!firstKnob)
                fi.margin.left = gapPx;
            firstKnob = false;
            fb.items.add(fi);
        }
        else
            fb.items.add(juce::FlexItem(*c).withFlex(0.f));
    }

    fb.performLayout(content.toFloat());
    syncChildrenEnabled();
}

} // namespace NamUi
