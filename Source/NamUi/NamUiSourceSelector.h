#pragma once

#include "NamUiColourPalette.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace NamUi
{

/**
 * Mock `SourceSelector`: INPUT / OUTPUT strip + LED + value popup.
 * Standalone: call `attachToAudioDeviceManager()` to drive channels from `AudioDeviceManager`.
 * AU/VST: manager is nullptr — stays manual (`setOptions` / `setSelectedValue`) or placeholder text.
 */
class NamUiSourceSelector : public juce::Component, private juce::ChangeListener
{
public:
    enum class Role
    {
        input,
        output
    };

    NamUiSourceSelector(const ColourTokens& palette, Role selectorRole);

    void setSideLabel(const juce::String& text);
    void setChannelActive(bool active);
    bool isChannelActive() const noexcept { return channelActive; }

    /** Manual mode: popup lists these strings. Ignored while device manager is attached. */
    void setOptions(juce::StringArray lines);
    void setSelectedValue(const juce::String& value);

    void attachToAudioDeviceManager(juce::AudioDeviceManager* manager);
    juce::AudioDeviceManager* getAttachedDeviceManager() const noexcept { return deviceManager; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;

    ~NamUiSourceSelector() override;

private:
    struct PopupLookAndFeel : juce::LookAndFeel_V4
    {
        explicit PopupLookAndFeel(const ColourTokens& palette);

        juce::Font getPopupMenuFont() override;
        void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
        void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                               bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                               const juce::String& shortcutKeyText, const juce::Drawable* icon,
                               const juce::Colour* textColour) override;
        int getPopupMenuBorderSize() override;

        const ColourTokens& tokens;
    };

    struct SourceOption
    {
        juce::String deviceName;
        juce::String label;
        int channelStart { 0 };
        int channelCount { 1 };
    };

    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void rebuildFromDeviceManager();
    void syncValueFromDeviceSetup();
    void applyPick(int optionIndex);
    void updateValueHover(const juce::Point<int>& pos);

    const ColourTokens& tokens;
    Role role;
    PopupLookAndFeel popupLookAndFeel { tokens };

    juce::String sideLabel { "INPUT" };
    bool channelActive { true };
    bool valueHovered { false };
    bool mirrored { false };

    juce::StringArray menuLines;
    juce::Array<SourceOption> menuOptions;
    juce::String currentValue { "-" };

    juce::AudioDeviceManager* deviceManager { nullptr };

    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> valueArea;

    static constexpr int kGapAfterHeader = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiSourceSelector)
};

} // namespace NamUi
