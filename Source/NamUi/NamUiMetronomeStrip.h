#pragma once

#include "NamUiColourPalette.h"
#include "NamUiStandardButton.h"
#include <functional>
#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace NamUi
{

class NamUiMetronomeStrip : public juce::Component
{
public:
    static constexpr int kDefaultWidth = 134;

    NamUiMetronomeStrip(const ColourTokens& palette, juce::AudioProcessorValueTreeState& state);

    void resized() override;

    void setOnControlsChanged(std::function<void()> callback);
    void setOnBpmChanged(std::function<void(int)> callback);

    /** Up/Down nudge BPM (+/-1, Shift +/-5). Returns false while the BPM label is being edited. */
    bool handleBpmArrowKey(const juce::KeyPress& key);

private:
    class BpmLabel : public juce::Label
    {
    public:
        BpmLabel();

    private:
        void editorShown(juce::TextEditor* editor) override;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BpmLabel)
    };

    class BpmArrowButton : public NamUiStandardButton
    {
    public:
        BpmArrowButton(const ColourTokens& palette, const juce::String& text, std::function<void(bool)> callback);

    private:
        void clicked(const juce::ModifierKeys& modifiers) override;

        std::function<void(bool)> onClickedWithShift;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BpmArrowButton)
    };

    class MetronomeIconButton : public juce::Button
    {
    public:
        explicit MetronomeIconButton(const ColourTokens& palette);

    private:
        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

        const ColourTokens& tokens;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetronomeIconButton)
    };

    const ColourTokens& tokens;
    juce::AudioProcessorValueTreeState& apvts;

    MetronomeIconButton toggleButton;
    juce::Slider bpmSlider;
    BpmLabel bpmLabel;
    BpmArrowButton bpmUpButton;
    BpmArrowButton bpmDownButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAttachment;

    std::function<void()> onControlsChanged;
    std::function<void(int)> onBpmChanged;

    void nudgeBpm(int delta);
    void syncBpmLabel();
    void commitBpmLabelText();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiMetronomeStrip)
};

} // namespace NamUi
