#include "NamUiMuteButton.h"
#include "NamUiFonts.h"

namespace NamUi
{

namespace
{
struct MuteLayout
{
    static constexpr float kFontH = 11.f;
    static constexpr float kGap = 5.f;
    static constexpr float kDotR = 2.5f;
    static constexpr float kPadX = 8.f;
    static constexpr float kPadY = 3.f;
};
} // namespace

NamUiMuteButton::NamUiMuteButton(const ColourTokens& palette)
    : juce::ToggleButton(""), tokens(palette)
{
    setButtonText("MUTE");
    setClickingTogglesState(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

int NamUiMuteButton::getIdealWidth() const
{
    auto font = Fonts::michroma(MuteLayout::kFontH);
    const float textW = font.getStringWidthFloat(getButtonText());
    const float contentW = 2.f * MuteLayout::kDotR + MuteLayout::kGap + textW;
    return juce::roundToInt(MuteLayout::kPadX * 2.f + contentW + 1.f);
}

int NamUiMuteButton::getIdealHeight() const
{
    auto font = Fonts::michroma(MuteLayout::kFontH);
    const float bodyH = juce::jmax(2.f * MuteLayout::kDotR, font.getHeight());
    return juce::roundToInt(MuteLayout::kPadY * 2.f + bodyH + 1.f);
}

void NamUiMuteButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsDown);

    const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const bool muted = getToggleState();
    const bool over = shouldDrawButtonAsHighlighted;
    const auto orange = tokens.accentOrange;

    const juce::Colour bg =
        muted ? juce::Colour::fromFloatRGBA(245.f / 255.f, 140.f / 255.f, 0.f, 0.15f)
              : (over ? juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 0.04f) : juce::Colours::transparentBlack);

    const juce::Colour borderCol = muted ? orange : (over ? tokens.border : tokens.borderDim);

    g.setColour(bg);
    g.fillRoundedRectangle(bounds, 2.f);

    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds, 2.f, 1.f);

    auto content = bounds.reduced(MuteLayout::kPadX, MuteLayout::kPadY);
    const float dotCx = content.getX() + MuteLayout::kDotR;
    const float dotCy = content.getCentreY();

    juce::Colour dotFill = muted ? orange : juce::Colours::transparentBlack;
    juce::Colour dotBorder = muted ? orange : tokens.textDim;

    if (muted)
    {
        g.setColour(juce::Colour::fromFloatRGBA(245.f / 255.f, 140.f / 255.f, 0.f, 0.35f));
        g.fillEllipse(dotCx - MuteLayout::kDotR - 2.f, dotCy - MuteLayout::kDotR - 2.f,
                      (MuteLayout::kDotR + 2.f) * 2.f, (MuteLayout::kDotR + 2.f) * 2.f);
    }

    g.setColour(dotFill);
    g.fillEllipse(dotCx - MuteLayout::kDotR, dotCy - MuteLayout::kDotR, MuteLayout::kDotR * 2.f,
                  MuteLayout::kDotR * 2.f);
    g.setColour(dotBorder);
    g.drawEllipse(dotCx - MuteLayout::kDotR, dotCy - MuteLayout::kDotR, MuteLayout::kDotR * 2.f,
                  MuteLayout::kDotR * 2.f, 1.f);

    g.setColour(muted ? orange : tokens.textDim);
    g.setFont(Fonts::michroma(MuteLayout::kFontH));
    const float textX = dotCx + MuteLayout::kDotR + MuteLayout::kGap;
    g.drawText(getButtonText(), textX, content.getY(), content.getRight() - textX, content.getHeight(),
               juce::Justification::centredLeft, false);
}

} // namespace NamUi
