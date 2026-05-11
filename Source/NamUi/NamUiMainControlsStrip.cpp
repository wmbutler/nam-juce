#include "NamUiMainControlsStrip.h"
#include "NamUiFonts.h"
#include <ff_meters/ff_meters.h>
#include <cmath>

namespace NamUi
{

namespace
{
static float sourceToNormalizedLevel(foleys::LevelMeterSource* source)
{
    if (source == nullptr)
        return 0.f;

    source->decayIfNeeded();

    float level = 0.f;
    for (int ch = 0; ch < source->getNumChannels(); ++ch)
        level = juce::jmax(level, source->getRMSLevel(ch));

    const auto db = juce::Decibels::gainToDecibels(level, -60.f);
    return juce::jlimit(0.f, 1.f, juce::jmap(db, -60.f, 0.f, 0.f, 1.f));
}
} // namespace

NamUiMainControlsStrip::MeterStub::MeterStub(const ColourTokens& palette, juce::String meterLabel)
    : tokens(palette), label(std::move(meterLabel))
{
}

void NamUiMainControlsStrip::MeterStub::setLevelNormalized(float newLevel)
{
    const auto clamped = juce::jlimit(0.f, 1.f, newLevel);
    if (std::abs(clamped - level) < 0.001f)
        return;

    level = clamped;
    repaint();
}

void NamUiMainControlsStrip::MeterStub::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    auto labelFont = Fonts::michroma(11.f);
    const int labelH = juce::roundToInt(labelFont.getHeight() * 1.25f);

    auto labelArea = bounds.removeFromBottom(labelH);
    bounds.removeFromBottom(4);

    static constexpr int kSegW = 16;
    static constexpr int kSegH = 5;
    static constexpr int kSegGap = 2;
    const int availableH = juce::jmax(0, bounds.getHeight() - 2);
    const int segments = juce::jmax(1, availableH / (kSegH + kSegGap));
    const int filled = juce::roundToInt(level * (float)segments);
    const int meterH = segments * kSegH + (segments - 1) * kSegGap;

    auto meter = juce::Rectangle<int>(bounds.getCentreX() - kSegW / 2,
                                      bounds.getBottom() - meterH,
                                      kSegW,
                                      meterH);

    for (int i = 0; i < segments; ++i)
    {
        const int y = meter.getBottom() - (i + 1) * kSegH - i * kSegGap;
        const auto pct = segments > 1 ? (float)i / (float)(segments - 1) : 0.f;
        const bool active = i < filled;

        juce::Colour segmentColour = tokens.meterGreen;
        if (pct > 0.85f)
            segmentColour = tokens.meterRed;
        else if (pct > 0.65f)
            segmentColour = tokens.meterYellow;

        auto seg = juce::Rectangle<float>((float)meter.getX(), (float)y, (float)kSegW, (float)kSegH);
        g.setColour(active ? segmentColour : juce::Colour(0xff1a1a22));
        g.fillRoundedRectangle(seg, 1.f);

        if (active && pct > 0.85f)
        {
            g.setColour(tokens.meterRed.withAlpha(0.35f));
            g.drawRoundedRectangle(seg.expanded(1.f), 1.f, 1.f);
        }
    }

    g.setColour(tokens.textMid);
    g.setFont(labelFont);
    g.drawText(label, labelArea, juce::Justification::centred, false);
}

NamUiMainControlsStrip::NamUiMainControlsStrip(const ColourTokens& palette)
    : tokens(palette),
      meterIn(palette, "IN"),
      meterOut(palette, "OUT"),
      knobInputLevel(palette, NamUiKnobReadoutKind::inputOutputLevelDb, "INPUT\nLEVEL", 36, 0.5f),
      knobOutputLevel(palette, NamUiKnobReadoutKind::inputOutputLevelDb, "OUTPUT\nLEVEL", 36, 0.5f),
      knobBass(palette, NamUiKnobReadoutKind::toneEqDb, "BASS", 40, 0.5f),
      knobMid(palette, NamUiKnobReadoutKind::toneEqDb, "MID", 40, 0.5f),
      knobTreble(palette, NamUiKnobReadoutKind::toneEqDb, "TREBLE", 40, 0.5f),
      knobGateOpen(palette, NamUiKnobReadoutKind::gateThresholdDb, "OPEN", 40, 0.f),
      knobGateClose(palette, NamUiKnobReadoutKind::gateThresholdDb, "CLOSE", 40, 1.f),
      toneGroup(palette, "TONE"),
      gateGroup(palette, "NOISE GATE"),
      muteInput(palette),
      muteOutput(palette)
{
    toneGroup.setInterKnobGapPixels(8.f);
    toneGroup.addAndMakeVisible(knobBass);
    toneGroup.addAndMakeVisible(knobMid);
    toneGroup.addAndMakeVisible(knobTreble);

    gateGroup.addAndMakeVisible(knobGateOpen);
    gateGroup.addAndMakeVisible(knobGateClose);

    addAndMakeVisible(meterIn);
    addAndMakeVisible(knobInputLevel);
    addAndMakeVisible(muteInput);
    addAndMakeVisible(toneGroup);
    addAndMakeVisible(gateGroup);
    addAndMakeVisible(meterOut);
    addAndMakeVisible(knobOutputLevel);
    addAndMakeVisible(muteOutput);

    auto notifyKnob = [this](double)
    {
        emitControlStateChanged();
    };
    knobInputLevel.onValueChange = notifyKnob;
    knobOutputLevel.onValueChange = notifyKnob;
    knobBass.onValueChange = notifyKnob;
    knobMid.onValueChange = notifyKnob;
    knobTreble.onValueChange = notifyKnob;
    knobGateOpen.onValueChange = [this](double value)
    {
        if (knobGateClose.getValueNormalized() < value)
            knobGateClose.setValueNormalized(value, juce::dontSendNotification);

        emitControlStateChanged();
    };
    knobGateClose.onValueChange = [this](double value)
    {
        if (value < knobGateOpen.getValueNormalized())
            knobGateClose.setValueNormalized(knobGateOpen.getValueNormalized(), juce::dontSendNotification);

        emitControlStateChanged();
    };

    toneGroup.onActiveChange = [this](bool)
    {
        emitControlStateChanged();
    };
    gateGroup.onActiveChange = [this](bool)
    {
        emitControlStateChanged();
    };

    muteInput.onClick = [this]
    {
        notifyAnyControlChanged();

        if (onMuteChanged)
            onMuteChanged(muteInput.getToggleState(), muteOutput.getToggleState());
    };
    muteOutput.onClick = [this]
    {
        notifyAnyControlChanged();

        if (onMuteChanged)
            onMuteChanged(muteInput.getToggleState(), muteOutput.getToggleState());
    };
}

void NamUiMainControlsStrip::setOnControlsChanged(std::function<void()> callback)
{
    onControlsChanged = std::move(callback);
}

void NamUiMainControlsStrip::setOnControlStateChanged(std::function<void(const ControlState&)> callback)
{
    onControlStateChanged = std::move(callback);
}

void NamUiMainControlsStrip::setOnMuteChanged(std::function<void(bool inputMuted, bool outputMuted)> callback)
{
    onMuteChanged = std::move(callback);
}

void NamUiMainControlsStrip::setControlState(const ControlState& state)
{
    knobInputLevel.setValueNormalized(state.inputLevel, juce::dontSendNotification);
    knobOutputLevel.setValueNormalized(state.outputLevel, juce::dontSendNotification);
    knobBass.setValueNormalized(state.bass, juce::dontSendNotification);
    knobMid.setValueNormalized(state.mid, juce::dontSendNotification);
    knobTreble.setValueNormalized(state.treble, juce::dontSendNotification);
    knobGateOpen.setValueNormalized(state.gateOpen, juce::dontSendNotification);
    knobGateClose.setValueNormalized(juce::jmax(state.gateOpen, state.gateClose), juce::dontSendNotification);
    toneGroup.setActive(state.toneActive);
    gateGroup.setActive(state.noiseGateActive);
}

void NamUiMainControlsStrip::setMeterSources(foleys::LevelMeterSource* inputSource, foleys::LevelMeterSource* outputSource)
{
    inputMeterSource = inputSource;
    outputMeterSource = outputSource;

    if (inputMeterSource != nullptr || outputMeterSource != nullptr)
        startTimerHz(30);
    else
        stopTimer();
}

void NamUiMainControlsStrip::timerCallback()
{
    meterIn.setLevelNormalized(sourceToNormalizedLevel(inputMeterSource));
    meterOut.setLevelNormalized(sourceToNormalizedLevel(outputMeterSource));
}

NamUiMainControlsStrip::ControlState NamUiMainControlsStrip::getControlState() const
{
    return { knobInputLevel.getValueNormalized(),
             knobOutputLevel.getValueNormalized(),
             knobBass.getValueNormalized(),
             knobMid.getValueNormalized(),
             knobTreble.getValueNormalized(),
             knobGateOpen.getValueNormalized(),
             knobGateClose.getValueNormalized(),
             toneGroup.isActive(),
             gateGroup.isActive() };
}

void NamUiMainControlsStrip::emitControlStateChanged()
{
    notifyAnyControlChanged();

    if (onControlStateChanged)
        onControlStateChanged(getControlState());
}

void NamUiMainControlsStrip::notifyAnyControlChanged()
{
    if (onControlsChanged)
        onControlsChanged();
}

void NamUiMainControlsStrip::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(tokens.bgPanel);
    g.fillRoundedRectangle(r, 2.f);
    g.setColour(tokens.border);
    g.drawRoundedRectangle(r, 2.f, 1.f);
}

void NamUiMainControlsStrip::layoutEdgeColumn(juce::Rectangle<int> column, juce::Component& meter, NamUiKnob& knob,
                                              NamUiMuteButton& mute, EdgeColumnAlign align)
{
    static constexpr int kInnerGap = 6;
    /// Worst-case I/O level readout (±20 dB, one decimal) — side column width targets this.
    static const juce::String kLevelReadoutWorstCase("+20.0dB");

    const int knobH = knob.getRecommendedHeight();
    const int muteH = mute.getIdealHeight();
    const int muteW = mute.getIdealWidth();
    const int kw = knob.getRecommendedWidthEnsuringReadout(kLevelReadoutWorstCase);

    const int stackFixedH = knobH + muteH + 2 * kInnerGap;
    const int meterH = juce::jmax(56, column.getHeight() - stackFixedH);

    const int stackH = meterH + stackFixedH;
    int yMeter = column.getBottom() - stackH;

    meter.setBounds(column.getX(), yMeter, column.getWidth(), meterH);
    yMeter += meterH + kInnerGap;

    const int knobX = (align == EdgeColumnAlign::left) ? column.getX() : (column.getRight() - kw);
    knob.setBounds(knobX, yMeter, kw, knobH);
    yMeter += knobH + kInnerGap;

    mute.setBounds(column.getCentreX() - muteW / 2, yMeter, muteW, muteH);
}

void NamUiMainControlsStrip::resized()
{
    auto area = getLocalBounds().withTrimmedLeft(10).withTrimmedRight(10).withTrimmedTop(10).withTrimmedBottom(8);
    /** Tighter gaps yield ~24px more for the middle column vs legacy 10px gaps (with side trim below). */
    static constexpr int kColGap = 6;
    static const juce::String kLevelReadoutWorstCase("+20.0dB");
    static constexpr int kSideColComfortPx = 4;
    static constexpr int kMidMinWidth = 120;
    /** Each I/O column sheds up to this many px (min width `sideContentW`) so TONE + NOISE GATE share ~24px more width. */
    static constexpr int kTrimEachSideForMidColumn = 12;

    const int sideContentW = juce::jmax(knobInputLevel.getRecommendedWidthEnsuringReadout(kLevelReadoutWorstCase),
                                        muteInput.getIdealWidth());
    const int desiredSide = sideContentW + kSideColComfortPx;
    const int maxEachSide =
        juce::jmax(1, (area.getWidth() - 2 * kColGap - kMidMinWidth) / 2);
    const int sideColBase = juce::jmin(desiredSide, maxEachSide);
    const int sideColW = juce::jmax(sideContentW, sideColBase - kTrimEachSideForMidColumn);

    juce::Rectangle<int> row = area;
    const auto leftCol = row.removeFromLeft(sideColW);
    row.removeFromLeft(kColGap);
    const auto rightCol = row.removeFromRight(sideColW);
    row.removeFromRight(kColGap);
    const auto midCol = row;

    layoutEdgeColumn(leftCol, meterIn, knobInputLevel, muteInput, EdgeColumnAlign::left);

    static constexpr int kGroupGap = 8;
    auto midArea = midCol;
    const int gh = juce::jmax(110, (midArea.getHeight() - kGroupGap) / 2);
    toneGroup.setBounds(midArea.removeFromTop(gh));
    midArea.removeFromTop(kGroupGap);
    gateGroup.setBounds(midArea);

    layoutEdgeColumn(rightCol, meterOut, knobOutputLevel, muteOutput, EdgeColumnAlign::right);
}

} // namespace NamUi
