#include "NamUiBrowserRow.h"
#include "NamUiFonts.h"

namespace NamUi
{

namespace
{
static juce::String prevGlyph()
{
    return juce::CharPointer_UTF8("\xe2\x97\x80"); // ◀
}

static juce::String nextGlyph()
{
    return juce::CharPointer_UTF8("\xe2\x96\xb6"); // ▶
}
} // namespace

static constexpr int kNavBtnWidth = 34;

NamUiBrowserRow::NamUiBrowserRow(const ColourTokens& palette)
    : tokens(palette), btnPrev(palette, StandardButtonStyle::browserNav, prevGlyph()),
      btnNext(palette, StandardButtonStyle::browserNav, nextGlyph()),
      btnActivate(palette, StandardButtonStyle::browserActivate, {})
{
    setOpaque(false);

    addAndMakeVisible(btnPrev);
    addAndMakeVisible(btnNext);
    addAndMakeVisible(btnActivate);
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(counterLabel);

    for (auto* l : {&titleLabel, &counterLabel})
    {
        l->setInterceptsMouseClicks(false, false);
        l->setMinimumHorizontalScale(1.f);
    }

    titleLabel.setJustificationType(juce::Justification::centredLeft);
    counterLabel.setJustificationType(juce::Justification::centred);

    wireButtons();
    refreshContents();
}

void NamUiBrowserRow::wireButtons()
{
    btnPrev.onClick = [this]
    {
        if (!rowDisabled && onPrevCb)
            onPrevCb();
    };
    btnNext.onClick = [this]
    {
        if (!rowDisabled && onNextCb)
            onNextCb();
    };
    btnActivate.onClick = [this]
    {
        if (!rowDisabled && onActivateCb)
            onActivateCb();
    };
}

int NamUiBrowserRow::effectiveTotal() const
{
    const int n = totalOverride > 0 ? totalOverride : items.size();
    return juce::jmax(0, n);
}

int NamUiBrowserRow::navButtonWidth()
{
    return kNavBtnWidth;
}

int NamUiBrowserRow::resolveCounterWidth() const
{
    const int tEff = effectiveTotal();
    const juce::String counterText =
        (items.isEmpty() || tEff <= 0) ? juce::String("0/0")
                                       : juce::String(itemIndex + 1) + "/" + juce::String(tEff);
    const float w = Fonts::shareTechMono(11.f, 0.f).getStringWidthFloat(counterText);
    return juce::roundToInt(w) + 21;
}

void NamUiBrowserRow::setRowDisabled(bool disabled)
{
    if (rowDisabled == disabled)
        return;
    rowDisabled = disabled;
    refreshContents();
    resized();
}

void NamUiBrowserRow::setItems(juce::StringArray lines)
{
    items = std::move(lines);
    itemIndex = juce::jlimit(0, juce::jmax(0, items.size() - 1), itemIndex);
    refreshContents();
    resized();
}

void NamUiBrowserRow::setBrowseIndex(int idx)
{
    const int maxIdx = juce::jmax(0, items.size() - 1);
    idx = juce::jlimit(0, maxIdx, idx);
    if (itemIndex == idx)
        return;
    itemIndex = idx;
    refreshContents();
}

void NamUiBrowserRow::setBrowseTotal(int totalOrZeroForItemsSize)
{
    totalOverride = juce::jmax(0, totalOrZeroForItemsSize);
    refreshContents();
    resized();
}

void NamUiBrowserRow::setShowActivateControl(bool show)
{
    if (showActivate == show)
        return;
    showActivate = show;
    btnActivate.setVisible(show);
    refreshContents();
    resized();
}

void NamUiBrowserRow::setActivateButtonText(const juce::String& label)
{
    activateLabel = label;
    btnActivate.setButtonText(label);
}

void NamUiBrowserRow::setActivateToggleState(bool isActive, juce::NotificationType nt)
{
    btnActivate.setToggleState(isActive, nt);
    btnActivate.repaint();
}

void NamUiBrowserRow::setOnActivate(std::function<void()> fn)
{
    onActivateCb = std::move(fn);
    setShowActivateControl(static_cast<bool>(onActivateCb));
}

void NamUiBrowserRow::setRightAccessory(juce::Component* component)
{
    if (rightAccessory == component)
        return;

    if (rightAccessory != nullptr)
        removeChildComponent(rightAccessory);

    rightAccessory = component;

    if (rightAccessory != nullptr)
        addAndMakeVisible(rightAccessory);

    resized();
}

void NamUiBrowserRow::refreshContents()
{
    const bool dim = rowDisabled;

    titleLabel.setColour(juce::Label::textColourId, dim ? tokens.textDim : tokens.textPrimary);
    titleLabel.setFont(Fonts::shareTechMono(11.f, 0.04f));

    if (items.isEmpty())
        titleLabel.setText("-", juce::dontSendNotification);
    else
        titleLabel.setText(items[juce::jlimit(0, items.size() - 1, itemIndex)], juce::dontSendNotification);

    const int tEff = effectiveTotal();
    if (items.isEmpty() || tEff <= 0)
        counterLabel.setText("0/0", juce::dontSendNotification);
    else
        counterLabel.setText(juce::String(itemIndex + 1) + "/" + juce::String(tEff),
                             juce::dontSendNotification);
    counterLabel.setFont(Fonts::shareTechMono(11.f, 0.f));
    counterLabel.setColour(juce::Label::textColourId, tokens.textMid);

    btnActivate.setButtonText(activateLabel.isEmpty() ? juce::String("ACTIVE") : activateLabel);

    btnPrev.setVisible(!rowDisabled);
    btnNext.setVisible(!rowDisabled);
    counterLabel.setVisible(!rowDisabled);

    btnPrev.setEnabled(!rowDisabled);
    btnNext.setEnabled(!rowDisabled);
    btnActivate.setEnabled(!rowDisabled && showActivate);
}

void NamUiBrowserRow::paint(juce::Graphics& g)
{
    g.fillAll(tokens.bgPanel);
    g.setColour(tokens.border);
    g.drawRect(getLocalBounds(), 1);

    if (!rowDisabled && counterLabel.isVisible())
    {
        const auto cb = counterLabel.getBounds();
        g.setColour(tokens.border);
        g.drawVerticalLine(cb.getX(), (float) cb.getY(), (float) cb.getBottom());
        g.drawVerticalLine(cb.getRight(), (float) cb.getY(), (float) cb.getBottom());
    }

    if (!rowDisabled && showActivate && rightAccessory != nullptr && btnActivate.getWidth() > 0)
    {
        g.setColour(tokens.border);
        g.drawVerticalLine(btnActivate.getBounds().getRight(), (float) getLocalBounds().getY(),
                           (float) getLocalBounds().getBottom());
    }
}

void NamUiBrowserRow::resized()
{
    const auto r = getLocalBounds();
    const int navW = navButtonWidth();
    const int counterW = rowDisabled ? 0 : resolveCounterWidth();
    const int activateW = (showActivate ? NamUiStandardButton::kBrowserActivateMinWidth : 0);
    const int accessoryW = (rightAccessory != nullptr ? juce::jmax(1, rightAccessory->getWidth()) : 0);

    if (rowDisabled)
    {
        btnPrev.setBounds(0, 0, 0, 0);
        btnNext.setBounds(0, 0, 0, 0);
        counterLabel.setBounds(0, 0, 0, 0);

        int xRight = r.getRight();

        if (rightAccessory != nullptr)
        {
            xRight -= accessoryW;
            rightAccessory->setBounds(xRight, r.getY(), accessoryW, r.getHeight());
        }

        if (showActivate)
        {
            xRight -= activateW;
            btnActivate.setBounds(xRight, r.getY(), activateW, r.getHeight());
            btnActivate.setVisible(true);
        }
        else
        {
            btnActivate.setBounds(0, 0, 0, 0);
            btnActivate.setVisible(false);
        }

        titleLabel.setJustificationType(juce::Justification::centred);
        titleLabel.setBounds(r.getX() + 8, r.getY(), juce::jmax(8, xRight - r.getX() - 16), r.getHeight());
        return;
    }

    btnActivate.setVisible(showActivate);
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    int x = r.getX();
    btnPrev.setBounds(x, r.getY(), navW, r.getHeight());
    x += navW;

    const int labelW =
        juce::jmax(20, r.getWidth() - navW - counterW - navW - activateW - accessoryW);

    titleLabel.setBounds(x, r.getY(), labelW, r.getHeight());
    x += labelW;

    counterLabel.setBounds(x, r.getY(), counterW, r.getHeight());
    x += counterW;

    btnNext.setBounds(x, r.getY(), navW, r.getHeight());
    x += navW;

    if (showActivate)
        btnActivate.setBounds(x, r.getY(), activateW, r.getHeight());
    else
        btnActivate.setBounds(0, 0, 0, 0);

    x += activateW;

    if (rightAccessory != nullptr)
        rightAccessory->setBounds(x, r.getY(), accessoryW, r.getHeight());
}

} // namespace NamUi
