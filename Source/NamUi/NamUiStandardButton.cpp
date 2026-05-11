#include "NamUiStandardButton.h"
#include "NamUiFonts.h"

namespace NamUi
{

NamUiStandardButton::NamUiStandardButton(const ColourTokens& palette, StandardButtonStyle buttonStyle,
                                         const juce::String& buttonText)
    : juce::Button({}), tokens(palette), style(buttonStyle)
{
    setButtonText(buttonText);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    if (style == StandardButtonStyle::browserActivate)
        setClickingTogglesState(true);
}

void NamUiStandardButton::setDanger(bool shouldBeDanger)
{
    if (danger == shouldBeDanger)
        return;
    danger = shouldBeDanger;
    repaint();
}

void NamUiStandardButton::setSoftCompact(bool compact)
{
    if (softCompact == compact)
        return;
    softCompact = compact;
    repaint();
}

void NamUiStandardButton::paintSoftLike(juce::Graphics& g, juce::Rectangle<float> bounds, bool enabled, bool down,
                                         float cornerRadius, const juce::Font& font, float padX, float padY,
                                         juce::Colour accent)
{
    // Disabled: grey outline reads on LCD (`bgDisplay`) better than all-amberDim (easy to miss).
    const juce::Colour borderCol = down ? accent : (enabled ? accent : tokens.border);
    const juce::Colour textCol = down ? tokens.bgDisplay : (enabled ? accent : tokens.amberDim);

    if (down)
    {
        g.setColour(accent);
        g.fillRoundedRectangle(bounds, cornerRadius);
    }

    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.f);

    g.setColour(textCol);
    g.setFont(font);
    g.drawText(getButtonText(), bounds.reduced(padX, padY), juce::Justification::centred, false);
}

void NamUiStandardButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                                      bool shouldDrawButtonAsDown)
{
    const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const bool enabled = isEnabled();
    const bool down = shouldDrawButtonAsDown;

    switch (style)
    {
    case StandardButtonStyle::softPreset:
    {
        juce::ignoreUnused(shouldDrawButtonAsHighlighted);
        const float fontH = 11.f;
        const float padX = softCompact ? 6.f : 12.f;
        const float padY = softCompact ? 3.f : 6.f;
        const juce::Colour accent = danger ? tokens.red : tokens.accentOrange;
        paintSoftLike(g, bounds, enabled, down, 2.f, Fonts::michroma(fontH), padX, padY, accent);
        break;
    }
    case StandardButtonStyle::presetNav:
        juce::ignoreUnused(shouldDrawButtonAsHighlighted);
        paintSoftLike(g, bounds, enabled, down, 1.f, Fonts::michroma(11.f), 6.f, 3.f, tokens.accentOrange);
        break;
    case StandardButtonStyle::shiftNav:
        juce::ignoreUnused(shouldDrawButtonAsHighlighted);
        paintSoftLike(g, bounds, enabled, down, 1.f, Fonts::shareTechMono(11.f, 0.12f), 6.f, 2.f,
                      tokens.accentOrange);
        break;
    case StandardButtonStyle::browserNav:
    {
        juce::ignoreUnused(shouldDrawButtonAsHighlighted);
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();

        g.setColour(tokens.border);
        g.fillRect(0.f, 0.f, 1.f, h);
        g.fillRect(w - 1.f, 0.f, 1.f, h);

        if (down)
        {
            g.setColour(tokens.accentOrange);
            g.fillRect(1.f, 0.f, w - 2.f, h);
        }

        g.setColour(down ? tokens.bgDisplay : tokens.amberDim);
        g.setFont(Fonts::shareTechMono(11.f, 0.f));
        g.drawText(getButtonText(), bounds.reduced(10.f, 0.f), juce::Justification::centred, false);
        break;
    }
    case StandardButtonStyle::browserActivate:
    {
        juce::ignoreUnused(shouldDrawButtonAsDown);

        const bool active = getToggleState();
        const bool lit = active && enabled;
        const bool over = enabled && shouldDrawButtonAsHighlighted;
        const auto gc = tokens.green;

        const juce::Colour bg =
            lit ? juce::Colour::fromFloatRGBA(57.f / 255.f, 233.f / 255.f, 123.f / 255.f, 0.12f)
                : (over ? juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 0.04f) : juce::Colours::transparentBlack);

        const juce::Colour edgeColour = lit ? gc : (over ? tokens.border : tokens.borderDim);
        const juce::Colour leftColour = lit ? gc : tokens.border;

        g.setColour(bg);
        g.fillRect(bounds);

        // Left separator (mock `borderLeft`)
        g.setColour(leftColour);
        g.fillRect(bounds.getX(), bounds.getY(), 1.f, bounds.getHeight());

        // Top / right / bottom (`border` minus left pixel handled visually)
        g.setColour(edgeColour);
        g.fillRect(bounds.getX() + 1.f, bounds.getY(), bounds.getWidth() - 1.f, 1.f);
        g.fillRect(bounds.getRight() - 1.f, bounds.getY() + 1.f, 1.f, bounds.getHeight() - 1.f);
        g.fillRect(bounds.getX() + 1.f, bounds.getBottom() - 1.f, bounds.getWidth() - 1.f, 1.f);

        const float padX = 5.f;
        const float gap = 6.f;
        const float dotR = 3.f;
        auto content = bounds.reduced(padX, 0.f);
        const auto labelFont = Fonts::michroma(11.f);
        const float textW = juce::jmin(labelFont.getStringWidthFloat(getButtonText()),
                                       juce::jmax(0.f, content.getWidth() - 2.f * dotR - gap));
        const float groupW = 2.f * dotR + gap + textW;
        const float groupX = content.getX() + juce::jmax(0.f, (content.getWidth() - groupW) * 0.5f);
        const float dotCx = groupX + dotR;
        const float dotCy = content.getCentreY();

        if (lit)
        {
            g.setColour(tokens.greenGlow.withMultipliedAlpha(0.45f));
            g.fillEllipse(dotCx - dotR - 2.f, dotCy - dotR - 2.f, (dotR + 2.f) * 2.f, (dotR + 2.f) * 2.f);
        }

        g.setColour(lit ? gc : juce::Colours::transparentBlack);
        g.fillEllipse(dotCx - dotR, dotCy - dotR, dotR * 2.f, dotR * 2.f);
        g.setColour(lit ? gc : tokens.textDim);
        g.drawEllipse(dotCx - dotR, dotCy - dotR, dotR * 2.f, dotR * 2.f, 1.f);

        const juce::Colour textCol = active ? gc : (enabled && over ? tokens.textMid : tokens.textDim);
        g.setColour(textCol);
        g.setFont(labelFont);
        const float textX = dotCx + dotR + gap;
        g.drawText(getButtonText(),
                   juce::Rectangle<float>(textX, content.getY(), content.getRight() - textX, content.getHeight()),
                   juce::Justification::centredLeft, false);
        break;
    }
    }
}

} // namespace NamUi
