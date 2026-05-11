#include "AboutDialogComponent.h"

namespace juce
{

AboutDialogComponent::AboutDialogComponent(String pluginVersionString)
    : versionString(std::move(pluginVersionString))
{
    setOpaque(true);

    okButton.onClick = [this]
    {
        if (auto* dw = findParentComponentOfClass<DialogWindow>())
            dw->exitModalState(0);
    };

    const auto linkFont = Font(FontOptions(12.0f));
    for (auto* link : { &authorLink, &namJuceLink, &namCoreLink })
    {
        link->setFont(linkFont, false, Justification::centred);
        link->setColour(HyperlinkButton::textColourId, Colours::blue);
        addAndMakeVisible(*link);
    }

    addAndMakeVisible(okButton);

    setSize(320, 260);
}

void AboutDialogComponent::paint(Graphics& g)
{
    g.fillAll(Colours::white);
    g.setColour(Colours::black);
    g.setFont(FontOptions(12.0f));

    auto textBounds = getLocalBounds().reduced(24, 20).withTrimmedBottom(44);
    auto y = textBounds.getY();
    constexpr int lineHeight = 18;

    g.drawText("Version " + versionString, textBounds.getX(), y, textBounds.getWidth(), lineHeight, Justification::centred);
    y += lineHeight + 8;

    g.drawText("Author", textBounds.getX(), y, textBounds.getWidth(), lineHeight, Justification::centred);
    y += (lineHeight * 2) + 8;

    g.setFont(Font(FontOptions(12.0f)).boldened());
    g.drawText("Acknowledgements", textBounds.getX(), y, textBounds.getWidth(), lineHeight, Justification::centred);
}

void AboutDialogComponent::resized()
{
    auto area = getLocalBounds().reduced(24, 16);
    okButton.setBounds(area.removeFromBottom(32).withSizeKeepingCentre(96, 28));

    auto textBounds = getLocalBounds().reduced(24, 20).withTrimmedBottom(44);
    auto y = textBounds.getY();
    constexpr int lineHeight = 18;

    y += lineHeight + 8;
    y += lineHeight;
    authorLink.setBounds(textBounds.getX(), y, textBounds.getWidth(), lineHeight);

    y += lineHeight + 8;
    y += lineHeight;
    namJuceLink.setBounds(textBounds.getX(), y, textBounds.getWidth(), lineHeight);
    y += lineHeight;
    namCoreLink.setBounds(textBounds.getX(), y, textBounds.getWidth(), lineHeight);
}

} // namespace juce
