#pragma once

#include "NamUiColourPalette.h"
#include "NamUiKnob.h"
#include "NamUiKnobGroup.h"
#include "NamUiMuteButton.h"
#include <functional>

namespace foleys
{
class LevelMeterSource;
}

namespace NamUi
{

/**
 * Mock main strip: IN meter + INPUT LEVEL + mute | TONE + NOISE GATE groups | OUT meter + OUTPUT LEVEL + mute.
 */
class NamUiMainControlsStrip : public juce::Component, private juce::Timer
{
public:
    struct ControlState
    {
        double inputLevel { 0.5 };
        double outputLevel { 0.5 };
        double bass { 0.5 };
        double mid { 0.5 };
        double treble { 0.5 };
        double gateOpen { 0. };
        double gateClose { 1. };
        bool toneActive { true };
        bool noiseGateActive { true };
    };

    explicit NamUiMainControlsStrip(const ColourTokens& palette);

    void paint(juce::Graphics& g) override;
    void resized() override;

    /** Fired after any knob value change or mute toggle — for preset dirty flags (demo / APVTS wiring). */
    void setOnControlsChanged(std::function<void()> callback);
    void setOnControlStateChanged(std::function<void(const ControlState&)> callback);
    void setOnMuteChanged(std::function<void(bool inputMuted, bool outputMuted)> callback);
    void setMeterSources(foleys::LevelMeterSource* inputSource, foleys::LevelMeterSource* outputSource);
    void setControlState(const ControlState& state);

private:
    enum class EdgeColumnAlign
    {
        left,
        right
    };

    void layoutEdgeColumn(juce::Rectangle<int> column, juce::Component& meter, NamUiKnob& knob, NamUiMuteButton& mute,
                          EdgeColumnAlign align);
    void timerCallback() override;
    ControlState getControlState() const;
    void emitControlStateChanged();
    void notifyAnyControlChanged();

    const ColourTokens& tokens;

    struct MeterStub : public juce::Component
    {
        MeterStub(const ColourTokens& palette, juce::String meterLabel);

        void paint(juce::Graphics& g) override;
        void setLevelNormalized(float newLevel);

        const ColourTokens& tokens;
        juce::String label;
        float level { 0.f };
    };

    MeterStub meterIn;
    MeterStub meterOut;

    NamUiKnob knobInputLevel;
    NamUiKnob knobOutputLevel;
    NamUiKnob knobBass;
    NamUiKnob knobMid;
    NamUiKnob knobTreble;
    NamUiKnob knobGateOpen;
    NamUiKnob knobGateClose;

    NamUiKnobGroup toneGroup;
    NamUiKnobGroup gateGroup;

    NamUiMuteButton muteInput;
    NamUiMuteButton muteOutput;

    foleys::LevelMeterSource* inputMeterSource { nullptr };
    foleys::LevelMeterSource* outputMeterSource { nullptr };

    std::function<void()> onControlsChanged;
    std::function<void(const ControlState&)> onControlStateChanged;
    std::function<void(bool inputMuted, bool outputMuted)> onMuteChanged;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamUiMainControlsStrip)
};

} // namespace NamUi
