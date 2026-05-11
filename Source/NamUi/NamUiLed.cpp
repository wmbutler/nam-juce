#include "NamUiLed.h"

namespace NamUi
{

NamUiLed::NamUiLed(const ColourTokens& palette)
    : tokens(palette), litColour(palette.green), glowColour(palette.greenGlow)
{
    setSize(kDefaultSide, kDefaultSide);
    setInterceptsMouseClicks(false, false);
}

void NamUiLed::setActive(bool shouldBeActive)
{
    if (active == shouldBeActive)
        return;
    active = shouldBeActive;
    repaint();
}

void NamUiLed::setLitColours(juce::Colour lit, juce::Colour glow)
{
    litColour = lit;
    glowColour = glow;
    repaint();
}

void NamUiLed::paint(juce::Graphics& g)
{
    const auto full = getLocalBounds().toFloat();
    const float cx = full.getCentreX();
    const float cy = full.getCentreY();
    const float halfSide = 0.5f * juce::jmin(full.getWidth(), full.getHeight());
    // Inset so a 1 px stroke sits inside the 10×10 box like the mock border.
    const float stroke = 1.f;
    const float r = juce::jmax(1.f, halfSide - stroke);

    if (active)
    {
        // Approximate CSS box-shadow: 0 0 6px 2px glow, 0 0 2px colour
        for (int pass = 2; pass >= 0; --pass)
        {
            const float extra = (float) pass * 2.f + 1.f;
            g.setColour(glowColour.withMultipliedAlpha(0.22f * (float) (pass + 1) / 3.f));
            g.fillEllipse(cx - r - extra, cy - r - extra, 2.f * (r + extra), 2.f * (r + extra));
        }

        juce::ColourGradient grad(litColour.brighter(0.12f), cx - r * 0.35f, cy - r * 0.35f, litColour.darker(0.18f),
                                  cx + r * 0.65f, cy + r * 0.65f, true);
        g.setGradientFill(grad);
        g.fillEllipse(cx - r, cy - r, 2.f * r, 2.f * r);

        g.setColour(litColour);
        g.drawEllipse(cx - r, cy - r, 2.f * r, 2.f * r, stroke);
    }
    else
    {
        g.setColour(tokens.ledInactiveFill);
        g.fillEllipse(cx - r, cy - r, 2.f * r, 2.f * r);
        g.setColour(tokens.ledInactiveBorder);
        g.drawEllipse(cx - r, cy - r, 2.f * r, 2.f * r, stroke);
    }
}

} // namespace NamUi
