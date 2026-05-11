#include "NamUiPresetPanel.h"
#include "NamUiFonts.h"

namespace NamUi
{
namespace
{
static constexpr float kPresetNameFontH = 34.f;
static constexpr float kPresetNameLineSpacing = 1.f;
static constexpr int kPresetNameTopLift = 5;

juce::Font presetNameFont()
{
    return Fonts::michroma(kPresetNameFontH);
}

/** Horizontal slack matches presetNav paint insets (~6px each side) + frame. */
int navBtnWidthForLabel(const juce::Font& f, const juce::String& label)
{
    return juce::roundToInt(f.getStringWidthFloat(label)) + 16;
}

void layoutNavTriple(juce::Rectangle<int>& nr, const juce::Font& navFont, const juce::String& leftLabel,
                     const juce::String& rightLabel, juce::Rectangle<int>& outLeft, juce::Rectangle<int>& outMid,
                     juce::Rectangle<int>& outRight)
{
    static constexpr int kMinCounter = 28;
    const int wL = navBtnWidthForLabel(navFont, leftLabel);
    const int wR = navBtnWidthForLabel(navFont, rightLabel);
    const int rowW = nr.getWidth();

    if (wL + wR + kMinCounter <= rowW)
    {
        outLeft = nr.removeFromLeft(wL);
        outRight = nr.removeFromRight(wR);
        outMid = nr;
        return;
    }

    const int w = juce::jmax(44, (rowW - kMinCounter) / 2);
    outLeft = nr.removeFromLeft(w);
    outRight = nr.removeFromRight(w);
    outMid = nr;
}

juce::StringArray wrapByCharacter(const juce::String& text, const juce::Font& font, float maxWidth, int maxLines)
{
    juce::StringArray lines;
    juce::String current;

    for (int i = 0; i < text.length(); ++i)
    {
        const auto ch = text[i];

        if (ch == '\n')
        {
            lines.add(current.trimEnd());
            current.clear();
            if (lines.size() >= maxLines)
                return lines;
            continue;
        }

        const auto candidate = current + juce::String::charToString(ch);
        if (current.isNotEmpty() && font.getStringWidthFloat(candidate) > maxWidth)
        {
            lines.add(current.trimEnd());
            current = juce::String::charToString(ch).trimStart();
            if (lines.size() >= maxLines)
                return lines;
        }
        else
        {
            current = candidate;
        }
    }

    if (current.isNotEmpty() && lines.size() < maxLines)
        lines.add(current.trimEnd());

    return lines;
}

juce::String normaliseEditorPresetName(juce::String text)
{
    return text.replace("\r\n", "\n").replaceCharacter('\r', '\n').trim();
}

void drawPresetNameWrapped(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& text,
                           const ColourTokens& tokens, juce::Colour colour)
{
    const auto font = presetNameFont();
    const float lineH = font.getHeight() * kPresetNameLineSpacing;
    const int maxLines = juce::jmax(1, juce::roundToInt((float)bounds.getHeight() / lineH));
    const auto lines = wrapByCharacter(text, font, (float)bounds.getWidth(), maxLines);

    g.setFont(font);
    g.setColour(tokens.amber.withAlpha(0.35f));
    float y = (float)bounds.getY() + 1.f;
    for (auto line : lines)
    {
        g.drawText(line, bounds.getX(), juce::roundToInt(y), bounds.getWidth(), juce::roundToInt(lineH),
                   juce::Justification::topLeft, false);
        y += lineH;
    }

    g.setColour(colour);
    y = (float)bounds.getY();
    for (auto line : lines)
    {
        g.drawText(line, bounds.getX(), juce::roundToInt(y), bounds.getWidth(), juce::roundToInt(lineH),
                   juce::Justification::topLeft, false);
        y += lineH;
    }
}
} // namespace

NamUiPresetPanel::NamUiPresetPanel(const ColourTokens& palette)
    : tokens(palette)
{
    setWantsKeyboardFocus(true);

    addAndMakeVisible(presetPrev);
    addAndMakeVisible(presetNext);

    addAndMakeVisible(btnNew);
    addAndMakeVisible(btnSave);
    addAndMakeVisible(btnRename);
    addAndMakeVisible(btnDelete);
    addAndMakeVisible(shiftLeft);
    addAndMakeVisible(shiftRight);

    addAndMakeVisible(namingCancel);
    addAndMakeVisible(namingSave);
    addChildComponent(namingEditor);

    btnNew.setSoftCompact(true);
    btnSave.setSoftCompact(true);
    btnRename.setSoftCompact(true);
    btnDelete.setSoftCompact(true);

    shiftLeft.setButtonText(juce::String::charToString((juce::juce_wchar)0x25C0));
    shiftRight.setButtonText(juce::String::charToString((juce::juce_wchar)0x25B6));

    presetPrev.onClick = [this]
    {
        grabKeyboardFocus();
        if (!namingMode && onPrevCb)
            onPrevCb();
    };
    presetNext.onClick = [this]
    {
        grabKeyboardFocus();
        if (!namingMode && onNextCb)
            onNextCb();
    };

    btnNew.onClick = [this]
    {
        grabKeyboardFocus();
        handleSoftNew();
    };
    btnSave.onClick = [this]
    {
        grabKeyboardFocus();
        handleSoftSaveClick();
    };
    btnRename.onClick = [this]
    {
        grabKeyboardFocus();
        handleSoftRename();
    };
    btnDelete.onClick = [this]
    {
        grabKeyboardFocus();
        handleSoftDeleteClick();
    };
    shiftLeft.onClick = [this]
    {
        grabKeyboardFocus();
        handleShiftLeft();
    };
    shiftRight.onClick = [this]
    {
        grabKeyboardFocus();
        handleShiftRight();
    };

    namingCancel.onClick = [this]
    {
        exitNamingMode();
        grabKeyboardFocus();
    };
    namingSave.onClick = [this]
    {
        commitNamingSave();
        grabKeyboardFocus();
    };

    namingEditor.setMultiLine(true, true);
    namingEditor.setReturnKeyStartsNewLine(true);
    namingEditor.setTabKeyUsedAsCharacter(false);
    namingEditor.setScrollbarsShown(false);
    namingEditor.setScrollToShowCursor(false);
    namingEditor.setCaretVisible(true);
    namingEditor.setPopupMenuEnabled(false);
    namingEditor.setInputRestrictions(48);
    namingEditor.setIndents(0, 0);
    namingEditor.setBorder(juce::BorderSize<int>());
    namingEditor.setJustification(juce::Justification::topLeft);
    namingEditor.setLineSpacing(1.f);
    namingEditor.setOpaque(false);
    namingEditor.setVisible(false);
    namingEditor.addListener(this);

    namingEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    namingEditor.setColour(juce::TextEditor::textColourId, tokens.amber);
    namingEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    namingEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    namingEditor.setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
    namingEditor.setColour(juce::TextEditor::highlightColourId, tokens.accentOrange.withAlpha(0.35f));
    namingEditor.setColour(juce::CaretComponent::caretColourId, tokens.accentOrange);
    namingEditor.setFont(presetNameFont());
    namingEditor.applyFontToAllText(presetNameFont(), true);

    namingCancel.setVisible(false);
    namingSave.setVisible(false);

    updateSoftButtonStates();
}

NamUiPresetPanel::~NamUiPresetPanel()
{
    namingEditor.removeListener(this);
}

void NamUiPresetPanel::textEditorTextChanged(juce::TextEditor& editor)
{
    juce::ignoreUnused(editor);
    repaint(nameBounds);
}

bool NamUiPresetPanel::keyPressed(const juce::KeyPress& key)
{
    if (namingMode || presetTotal <= 1)
        return false;

    if (key == juce::KeyPress::leftKey)
    {
        if (onPrevCb)
            onPrevCb();
        return true;
    }

    if (key == juce::KeyPress::rightKey)
    {
        if (onNextCb)
            onNextCb();
        return true;
    }

    return false;
}

void NamUiPresetPanel::mouseDown(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    grabKeyboardFocus();
}

void NamUiPresetPanel::setPresetName(juce::String name)
{
    presetName = std::move(name);
    repaint();
}

void NamUiPresetPanel::setCounter(int indexZeroBased, int total)
{
    presetIndex = indexZeroBased;
    presetTotal = total;
    clearSaveArm();
    clearDeleteArm();
    updateSoftButtonStates();
    repaint();
}

void NamUiPresetPanel::setHasLoadedPreset(bool loaded)
{
    presetLoadedFlag = loaded;
    updateSoftButtonStates();
}

void NamUiPresetPanel::setPresetDirty(bool dirty)
{
    presetDirtyFlag = dirty;
    if (!dirty)
    {
        clearSaveArm();
        clearDeleteArm();
    }
    updateSoftButtonStates();
}

void NamUiPresetPanel::setOnPrev(std::function<void()> callback)
{
    onPrevCb = std::move(callback);
}

void NamUiPresetPanel::setOnNext(std::function<void()> callback)
{
    onNextCb = std::move(callback);
}

void NamUiPresetPanel::setOnSave(std::function<void()> callback)
{
    onSaveCb = std::move(callback);
}

void NamUiPresetPanel::setOnNewPreset(std::function<void(juce::String)> callback)
{
    onNewPresetCb = std::move(callback);
}

void NamUiPresetPanel::setOnRenamePreset(std::function<void(juce::String)> callback)
{
    onRenamePresetCb = std::move(callback);
}

void NamUiPresetPanel::setOnDeletePreset(std::function<void()> callback)
{
    onDeletePresetCb = std::move(callback);
}

void NamUiPresetPanel::setOnMovePreset(std::function<void(int, int)> callback)
{
    onMovePresetCb = std::move(callback);
}

void NamUiPresetPanel::updateSoftButtonStates()
{
    const bool naming = namingMode;
    const bool hasBank = presetTotal > 0;
    const bool loaded = presetLoadedFlag && hasBank;

    btnNew.setEnabled(!naming && (presetDirtyFlag || presetTotal == 0));
    btnSave.setEnabled(!naming && presetDirtyFlag && presetLoadedFlag && hasBank);
    btnRename.setEnabled(!naming && loaded);
    btnDelete.setEnabled(!naming && loaded);
    shiftLeft.setEnabled(!naming && loaded && presetIndex > 0);
    shiftRight.setEnabled(!naming && loaded && presetIndex < presetTotal - 1);

    const bool multi = presetTotal > 1;
    presetPrev.setEnabled(!naming && multi);
    presetNext.setEnabled(!naming && multi);

    namingCancel.setEnabled(naming);
    namingSave.setEnabled(naming);
}

void NamUiPresetPanel::syncSaveDeleteButtonLabels()
{
    btnSave.setButtonText(saveArmed ? "WRITE" : "SAVE");
    btnDelete.setButtonText(deleteArmed ? "CONFIRM" : "DELETE");
}

void NamUiPresetPanel::armSaveSecondStep()
{
    saveArmed = true;
    ++saveArmGeneration;
    const uint32_t gen = saveArmGeneration;
    btnSave.setDanger(true);
    syncSaveDeleteButtonLabels();

    auto safe = juce::Component::SafePointer<NamUiPresetPanel>(this);
    juce::Timer::callAfterDelay(3000, [safe, gen]()
    {
        if (safe == nullptr)
            return;
        if (safe->saveArmGeneration != gen)
            return;
        safe->clearSaveArm();
    });
}

void NamUiPresetPanel::clearSaveArm()
{
    saveArmed = false;
    ++saveArmGeneration;
    btnSave.setDanger(false);
    syncSaveDeleteButtonLabels();
}

void NamUiPresetPanel::armDeleteSecondStep()
{
    deleteArmed = true;
    ++deleteArmGeneration;
    const uint32_t gen = deleteArmGeneration;
    btnDelete.setDanger(true);
    syncSaveDeleteButtonLabels();

    auto safe = juce::Component::SafePointer<NamUiPresetPanel>(this);
    juce::Timer::callAfterDelay(3000, [safe, gen]()
    {
        if (safe == nullptr)
            return;
        if (safe->deleteArmGeneration != gen)
            return;
        safe->clearDeleteArm();
    });
}

void NamUiPresetPanel::clearDeleteArm()
{
    deleteArmed = false;
    ++deleteArmGeneration;
    btnDelete.setDanger(false);
    syncSaveDeleteButtonLabels();
}

void NamUiPresetPanel::showWrittenFlash()
{
    flashText = "WRITTEN";
    ++flashGeneration;
    const uint32_t gen = flashGeneration;
    repaint();

    auto safe = juce::Component::SafePointer<NamUiPresetPanel>(this);
    juce::Timer::callAfterDelay(1000, [safe, gen]()
    {
        if (safe == nullptr)
            return;
        if (safe->flashGeneration != gen)
            return;
        safe->flashText.clear();
        safe->repaint();
    });
}

void NamUiPresetPanel::enterNamingMode(bool forNewPreset)
{
    clearSaveArm();
    clearDeleteArm();
    flashText.clear();

    namingMode = true;
    namingIsNew = forNewPreset;

    const auto editFont = presetNameFont();
    namingEditor.setFont(editFont);
    const auto editText = forNewPreset ? juce::String() : presetName;
    namingEditor.setText(editText, juce::dontSendNotification);
    namingEditor.applyFontToAllText(editFont, true);
    namingEditor.setCaretPosition(0);
    namingEditor.setVisible(true);

    namingCancel.setVisible(true);
    namingSave.setVisible(true);

    presetPrev.setVisible(false);
    presetNext.setVisible(false);

    btnNew.setEnabled(false);
    btnSave.setEnabled(false);
    btnRename.setEnabled(false);
    btnDelete.setEnabled(false);
    shiftLeft.setEnabled(false);
    shiftRight.setEnabled(false);

    namingCancel.setEnabled(true);
    namingSave.setEnabled(true);

    resized();
    namingEditor.toFront(false);
    namingEditor.grabKeyboardFocus();
    namingEditor.moveCaretToEnd();
    repaint();
}

void NamUiPresetPanel::exitNamingMode()
{
    if (!namingMode)
        return;

    namingMode = false;
    namingEditor.setVisible(false);

    namingCancel.setVisible(false);
    namingSave.setVisible(false);

    presetPrev.setVisible(true);
    presetNext.setVisible(true);

    updateSoftButtonStates();
    resized();
    repaint();
}

void NamUiPresetPanel::commitNamingSave()
{
    if (!namingMode)
        return;

    auto name = normaliseEditorPresetName(namingEditor.getText());
    if (name.isEmpty())
        name = "UNTITLED";

    exitNamingMode();

    if (namingIsNew)
    {
        if (onNewPresetCb)
            onNewPresetCb(name);
    }
    else
    {
        if (onRenamePresetCb)
            onRenamePresetCb(name);
    }
}

void NamUiPresetPanel::handleSoftNew()
{
    if (namingMode)
        return;
    enterNamingMode(true);
}

void NamUiPresetPanel::handleSoftRename()
{
    if (namingMode)
        return;
    enterNamingMode(false);
}

void NamUiPresetPanel::handleSoftSaveClick()
{
    if (namingMode)
        return;

    if (saveArmed)
    {
        clearSaveArm();
        if (onSaveCb)
            onSaveCb();
        showWrittenFlash();
        return;
    }

    if (presetDirtyFlag && presetLoadedFlag && presetTotal > 0)
        armSaveSecondStep();
}

void NamUiPresetPanel::handleSoftDeleteClick()
{
    if (namingMode)
        return;

    if (deleteArmed)
    {
        clearDeleteArm();
        if (onDeletePresetCb)
            onDeletePresetCb();
        return;
    }

    armDeleteSecondStep();
}

void NamUiPresetPanel::handleShiftLeft()
{
    if (namingMode || presetIndex <= 0 || !onMovePresetCb)
        return;
    onMovePresetCb(presetIndex, presetIndex - 1);
}

void NamUiPresetPanel::handleShiftRight()
{
    if (namingMode || presetIndex >= presetTotal - 1 || !onMovePresetCb)
        return;
    onMovePresetCb(presetIndex, presetIndex + 1);
}

void NamUiPresetPanel::resized()
{
    auto inner = getLocalBounds().reduced(1);
    auto body =
        inner.withTrimmedTop(kPadY).withTrimmedBottom(kPadY).withTrimmedLeft(kPadX).withTrimmedRight(kPadRight);

    softColumnBounds = body.removeFromRight(kSoftColumnWidth);
    mainColumnBounds = body;

    auto mainColumn = mainColumnBounds.withTrimmedRight(kDividerGap);
    auto navRow = mainColumn.removeFromBottom(kNavRowH);
    nameBounds = mainColumn.withTrimmedTop(-kPresetNameTopLift);

    Fonts::ensureLoaded();
    const auto navFont = Fonts::michroma(11.f);

    if (namingMode)
    {
        namingEditor.setBounds(nameBounds);

        auto nr = navRow;
        juce::Rectangle<int> cancelR, saveR, counterR;
        layoutNavTriple(nr, navFont, "CANCEL", "SAVE", cancelR, counterR, saveR);
        namingCancel.setBounds(cancelR);
        namingSave.setBounds(saveR);
        counterBounds = counterR;

        presetPrev.setBounds({});
        presetNext.setBounds({});
    }
    else
    {
        namingEditor.setBounds({});

        auto nr = navRow;
        juce::Rectangle<int> prevR, nextR, counterR;
        layoutNavTriple(nr, navFont, "PREV", "NEXT", prevR, counterR, nextR);
        presetPrev.setBounds(prevR);
        presetNext.setBounds(nextR);
        counterBounds = counterR;

        namingCancel.setBounds({});
        namingSave.setBounds({});
    }

    auto soft = softColumnBounds.withTrimmedLeft(kDividerGap).withTrimmedRight(1).withTrimmedTop(1).withTrimmedBottom(1);
    const int btnH = kPresetPanelBtnH;
    const int btnStackH = 4 * btnH + 3 * kSoftGap;
    const int maxShift = juce::jmax(0, soft.getHeight() - btnStackH - kSoftGap);
    const int shiftH = juce::jmin(btnH, maxShift);
    auto shiftRow = soft.removeFromBottom(shiftH);

    btnNew.setBounds(soft.removeFromTop(btnH));
    soft.removeFromTop(kSoftGap);
    btnSave.setBounds(soft.removeFromTop(btnH));
    soft.removeFromTop(kSoftGap);
    btnRename.setBounds(soft.removeFromTop(btnH));
    soft.removeFromTop(kSoftGap);
    btnDelete.setBounds(soft.removeFromTop(btnH));

    const int halfGap = 4;
    auto leftShift = shiftRow.removeFromLeft(juce::jmax(1, shiftRow.getWidth() / 2 - halfGap / 2));
    shiftRow.removeFromLeft(halfGap);
    shiftLeft.setBounds(leftShift);
    shiftRight.setBounds(shiftRow);

    // Keep the soft column above PREV/NEXT/name painting stack (defensive z-order).
    btnNew.toFront(false);
    btnSave.toFront(false);
    btnRename.toFront(false);
    btnDelete.toFront(false);
    shiftLeft.toFront(false);
    shiftRight.toFront(false);

    if (namingMode)
        namingEditor.toFront(false);
}

void NamUiPresetPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);

    g.setColour(tokens.bgDisplay);
    g.fillRoundedRectangle(bounds, 2.f);

    {
        juce::Graphics::ScopedSaveState ss(g);
        g.reduceClipRegion(getLocalBounds());
        g.setColour(juce::Colours::black.withAlpha(0.08f));
        for (int y = 0; y < getHeight(); y += 4)
            g.fillRect(0.f, (float)(y + 2), (float)getWidth(), 2.f);
    }

    g.setColour(tokens.amberDim);
    g.drawRoundedRectangle(bounds, 2.f, 1.f);

    if (softColumnBounds.getWidth() > 0)
    {
        const float x = (float)softColumnBounds.getX();
        const float y1 = (float)softColumnBounds.getY() + (float)kDividerMarginY;
        const float y2 = (float)softColumnBounds.getBottom() - (float)kDividerMarginY;
        g.setColour(tokens.amberDim);
        g.drawLine(x, y1, x, y2, 1.f);
    }

    static const juce::String kEmDash = juce::String(juce::CharPointer_UTF8("\xe2\x80\x94"));
    juce::String counterText =
        presetTotal > 0 ? juce::String(presetIndex + 1) + "/" + juce::String(presetTotal) : kEmDash;

    g.setColour(tokens.accentOrange);
    g.setFont(Fonts::shareTechMono(11.f, 0.1f));
    g.drawText(counterText, counterBounds, juce::Justification::centred, false);

    if (flashText.isNotEmpty())
    {
        drawPresetNameWrapped(g, nameBounds, flashText, tokens, tokens.accentOrange);
    }
    else if (!namingMode)
    {
        drawPresetNameWrapped(g, nameBounds, presetName, tokens, tokens.amber);
    }
}

} // namespace NamUi
