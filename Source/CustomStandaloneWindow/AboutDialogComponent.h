#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace juce
{

/** Plain centered About panel: black 12pt text on white; OK dismisses the hosting DialogWindow. */
class AboutDialogComponent final : public Component
{
public:
    explicit AboutDialogComponent(String pluginVersionString);

    void paint(Graphics& g) override;
    void resized() override;

private:
    String versionString;
    TextButton okButton { "OK" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutDialogComponent)
};

} // namespace juce
