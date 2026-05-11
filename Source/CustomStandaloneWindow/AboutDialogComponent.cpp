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

    addAndMakeVisible(okButton);

    setSize(320, 260);
}

void AboutDialogComponent::paint(Graphics& g)
{
    g.fillAll(Colours::white);
    g.setColour(Colours::black);
    g.setFont(FontOptions(12.0f));

    const String lines[] = {
        "Version " + versionString,
        {},
        "Author",
        "wmbutler",
        {},
        "Derivative works",
        "Tr3m/nam-juce",
        "sdatkinson/NeuralAmpModelerCore",
    };

    auto textBounds = getLocalBounds().reduced(24, 20).withTrimmedBottom(44);
    auto y = textBounds.getY();
    constexpr int lineHeight = 18;

    for (const auto& line : lines)
    {
        if (line.isEmpty())
        {
            y += 8;
            continue;
        }

        g.drawText(line, textBounds.getX(), y, textBounds.getWidth(), lineHeight, Justification::centred);
        y += lineHeight;
    }
}

void AboutDialogComponent::resized()
{
    auto area = getLocalBounds().reduced(24, 16);
    okButton.setBounds(area.removeFromBottom(32).withSizeKeepingCentre(96, 28));
}

} // namespace juce
