#include "NamUiMetronomeStrip.h"
#include "NamUiFonts.h"

namespace NamUi
{

namespace
{
static juce::String upGlyph()
{
    return juce::CharPointer_UTF8("\xe2\x96\xb2"); // ▲
}

static juce::String downGlyph()
{
    return juce::CharPointer_UTF8("\xe2\x96\xbc"); // ▼
}

static juce::Path tablerMetronomePath()
{
    auto body = juce::Drawable::parseSVGPath("M14.153 8.188l-.72 -3.236a2.493 2.493 0 0 0 -4.867 0l-3.025 13.614a2 2 0 0 0 1.952 2.434h7.014a2 2 0 0 0 1.952 -2.434l-.524 -2.357m-4.935 1.791l9 -13");
    body.addPath(juce::Drawable::parseSVGPath("M19 5a1 1 0 1 0 2 0a1 1 0 1 0 -2 0"));
    return body;
}
} // namespace

NamUiMetronomeStrip::BpmLabel::BpmLabel()
{
    setEditable(true, false, false);
    setMouseCursor(juce::MouseCursor::IBeamCursor);
    setKeyboardType(juce::TextInputTarget::decimalKeyboard);
}

void NamUiMetronomeStrip::BpmLabel::editorShown(juce::TextEditor* editor)
{
    if (editor == nullptr)
        return;

    editor->setInputRestrictions(3, "0123456789");
    editor->setSelectAllWhenFocused(true);
    editor->setJustification(juce::Justification::centred);
    editor->setIndents(0, 0);
    editor->selectAll();
}

NamUiMetronomeStrip::BpmArrowButton::BpmArrowButton(const ColourTokens& palette, const juce::String& text,
                                                    std::function<void(bool)> callback)
    : NamUiStandardButton(palette, StandardButtonStyle::browserNav, text),
      onClickedWithShift(std::move(callback))
{
}

void NamUiMetronomeStrip::BpmArrowButton::clicked(const juce::ModifierKeys& modifiers)
{
    if (onClickedWithShift)
        onClickedWithShift(modifiers.isShiftDown());
}

NamUiMetronomeStrip::MetronomeIconButton::MetronomeIconButton(const ColourTokens& palette)
    : juce::Button({}), tokens(palette)
{
    setClickingTogglesState(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void NamUiMetronomeStrip::MetronomeIconButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                                                           bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsDown);

    const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const bool enabled = isEnabled();
    const bool active = getToggleState();
    const bool lit = active && enabled;
    const bool over = enabled && shouldDrawButtonAsHighlighted;
    const auto gc = tokens.green;

    const juce::Colour bg =
        lit ? juce::Colour::fromFloatRGBA(57.f / 255.f, 233.f / 255.f, 123.f / 255.f, 0.12f)
            : (over ? juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 0.04f) : juce::Colours::transparentBlack);
    const juce::Colour edgeColour = lit ? gc : (over ? tokens.border : tokens.borderDim);
    const juce::Colour leftColour = lit ? gc : tokens.border;
    const juce::Colour iconColour = lit ? gc : (over ? tokens.textMid : tokens.textDim);

    g.setColour(bg);
    g.fillRect(bounds);

    g.setColour(leftColour);
    g.fillRect(bounds.getX(), bounds.getY(), 1.f, bounds.getHeight());
    g.setColour(edgeColour);
    g.fillRect(bounds.getX() + 1.f, bounds.getY(), bounds.getWidth() - 1.f, 1.f);
    g.fillRect(bounds.getRight() - 1.f, bounds.getY() + 1.f, 1.f, bounds.getHeight() - 1.f);
    g.fillRect(bounds.getX() + 1.f, bounds.getBottom() - 1.f, bounds.getWidth() - 1.f, 1.f);

    auto icon = bounds.reduced(7.f, 7.f);
    const auto scale = juce::jmin(icon.getWidth() / 24.f, icon.getHeight() / 24.f);
    const auto iconX = icon.getX() + (icon.getWidth() - 24.f * scale) * 0.5f;
    const auto iconY = icon.getY() + (icon.getHeight() - 24.f * scale) * 0.5f;

    auto metronome = tablerMetronomePath();
    metronome.applyTransform(juce::AffineTransform::scale(scale).translated(iconX, iconY));

    g.setColour(iconColour);
    g.strokePath(metronome, juce::PathStrokeType(2.f * scale,
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
}

NamUiMetronomeStrip::NamUiMetronomeStrip(const ColourTokens& palette, juce::AudioProcessorValueTreeState& state)
    : tokens(palette),
      apvts(state),
      toggleButton(palette),
      bpmUpButton(palette, upGlyph(), [this](bool shiftDown) { nudgeBpm(shiftDown ? 5 : 1); }),
      bpmDownButton(palette, downGlyph(), [this](bool shiftDown) { nudgeBpm(shiftDown ? -5 : -1); })
{
    addAndMakeVisible(toggleButton);
    addAndMakeVisible(bpmSlider);
    addAndMakeVisible(bpmLabel);
    addAndMakeVisible(bpmUpButton);
    addAndMakeVisible(bpmDownButton);

    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    bpmSlider.setNumDecimalPlacesToDisplay(0);
    bpmSlider.setScrollWheelEnabled(false);
    bpmSlider.setVisible(false);

    bpmLabel.setJustificationType(juce::Justification::centred);
    bpmLabel.setColour(juce::Label::textColourId, tokens.textPrimary);
    bpmLabel.setColour(juce::Label::backgroundColourId, tokens.bgDisplay);
    bpmLabel.setColour(juce::Label::outlineColourId, tokens.border);
    bpmLabel.setColour(juce::Label::textWhenEditingColourId, tokens.textPrimary);
    bpmLabel.setColour(juce::Label::backgroundWhenEditingColourId, tokens.bgDisplay);
    bpmLabel.setColour(juce::Label::outlineWhenEditingColourId, tokens.green);
    bpmLabel.setFont(Fonts::shareTechMono(13.f, 0.04f));
    bpmLabel.setMinimumHorizontalScale(1.f);
    bpmLabel.onTextChange = [this]
    {
        commitBpmLabelText();
    };

    toggleButton.onClick = [this]
    {
        if (onControlsChanged)
            onControlsChanged();
    };

    bpmSlider.onValueChange = [this]
    {
        syncBpmLabel();

        if (onControlsChanged)
            onControlsChanged();

        if (onBpmChanged)
            onBpmChanged(juce::roundToInt(bpmSlider.getValue()));
    };

    toggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "METRONOME_ON_ID", toggleButton);
    bpmAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "METRONOME_BPM_ID", bpmSlider);
    syncBpmLabel();
}

void NamUiMetronomeStrip::setOnControlsChanged(std::function<void()> callback)
{
    onControlsChanged = std::move(callback);
}

void NamUiMetronomeStrip::setOnBpmChanged(std::function<void(int)> callback)
{
    onBpmChanged = std::move(callback);
}

bool NamUiMetronomeStrip::handleBpmArrowKey(const juce::KeyPress& key)
{
    if (bpmLabel.isBeingEdited())
        return false;

    const int step = key.getModifiers().isShiftDown() ? 5 : 1;

    if (key == juce::KeyPress::upKey)
    {
        nudgeBpm(step);
        return true;
    }

    if (key == juce::KeyPress::downKey)
    {
        nudgeBpm(-step);
        return true;
    }

    return false;
}

void NamUiMetronomeStrip::nudgeBpm(int delta)
{
    const auto nextValue = juce::jlimit(40.0, 240.0, bpmSlider.getValue() + (double)delta);
    bpmSlider.setValue(nextValue, juce::sendNotificationSync);
}

void NamUiMetronomeStrip::syncBpmLabel()
{
    bpmLabel.setText(juce::String(juce::roundToInt(bpmSlider.getValue())), juce::dontSendNotification);
}

void NamUiMetronomeStrip::commitBpmLabelText()
{
    auto text = bpmLabel.getText(false).trim();
    const int currentBpm = juce::roundToInt(bpmSlider.getValue());

    if (text.isEmpty() || !text.containsOnly("0123456789"))
    {
        syncBpmLabel();
        return;
    }

    const int bpm = juce::jlimit(40, 240, text.getIntValue());

    if (bpm == currentBpm)
    {
        syncBpmLabel();
        return;
    }

    bpmSlider.setValue((double)bpm, juce::sendNotificationSync);
}

void NamUiMetronomeStrip::resized()
{
    auto area = getLocalBounds();

    toggleButton.setBounds(area.removeFromLeft(40));
    area.removeFromLeft(6);
    bpmSlider.setBounds(0, 0, 0, 0);

    auto rightArea = area.removeFromLeft(70);
    bpmLabel.setBounds(rightArea.removeFromTop(rightArea.getHeight() / 2));

    auto arrowRow = rightArea;
    bpmDownButton.setBounds(arrowRow.removeFromLeft(arrowRow.getWidth() / 2));
    bpmUpButton.setBounds(arrowRow);
}

} // namespace NamUi
