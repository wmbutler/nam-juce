#include "NamUiSourceSelector.h"
#include "NamUiFonts.h"

namespace NamUi
{

namespace
{
static juce::String makeInputLabel(const juce::String& deviceName, const juce::String& channelName)
{
    return deviceName + " — " + channelName;
}

static juce::String makeOutputLabel(const juce::String& deviceName, const juce::StringArray& channelNames, int channelStart)
{
    if (channelStart + 1 < channelNames.size())
        return deviceName + " — " + channelNames[channelStart] + " / " + channelNames[channelStart + 1];

    return deviceName + " — " + channelNames[channelStart];
}
} // namespace

NamUiSourceSelector::PopupLookAndFeel::PopupLookAndFeel(const ColourTokens& palette)
    : tokens(palette)
{
    setColour(juce::PopupMenu::backgroundColourId, tokens.bgDisplay);
    setColour(juce::PopupMenu::textColourId, tokens.textMid);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, tokens.amberGlow2);
    setColour(juce::PopupMenu::highlightedTextColourId, tokens.amber);
}

juce::Font NamUiSourceSelector::PopupLookAndFeel::getPopupMenuFont()
{
    return Fonts::shareTechMono(12.f, 0.f);
}

void NamUiSourceSelector::PopupLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(tokens.bgDisplay);
    g.setColour(tokens.border);
    g.drawRect(0, 0, width, height, 1);
}

void NamUiSourceSelector::PopupLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                                              bool isSeparator, bool isActive, bool isHighlighted,
                                                              bool isTicked, bool hasSubMenu, const juce::String& text,
                                                              const juce::String& shortcutKeyText,
                                                              const juce::Drawable* icon,
                                                              const juce::Colour* textColour)
{
    juce::ignoreUnused(shortcutKeyText, icon, hasSubMenu);

    if (isSeparator)
    {
        g.setColour(tokens.borderDim);
        g.fillRect(area.withHeight(1).withCentre(area.getCentre()));
        return;
    }

    auto row = area.reduced(1, 0);
    if (isHighlighted)
    {
        g.setColour(tokens.amberGlow2);
        g.fillRect(row);
    }

    const auto itemTextColour = textColour != nullptr ? *textColour : (isTicked ? tokens.amber : tokens.textMid);
    g.setColour(isActive ? itemTextColour : tokens.textDim);
    g.setFont(getPopupMenuFont());
    g.drawText(text, row.reduced(9, 0), juce::Justification::centredLeft, true);
}

int NamUiSourceSelector::PopupLookAndFeel::getPopupMenuBorderSize()
{
    return 1;
}

NamUiSourceSelector::NamUiSourceSelector(const ColourTokens& palette, Role selectorRole)
    : tokens(palette), role(selectorRole), mirrored(selectorRole == Role::output)
{
    setInterceptsMouseClicks(true, true);
    setSideLabel(selectorRole == Role::input ? "INPUT" : "OUTPUT");
}

NamUiSourceSelector::~NamUiSourceSelector()
{
    attachToAudioDeviceManager(nullptr);
}

void NamUiSourceSelector::setSideLabel(const juce::String& text)
{
    sideLabel = text;
    repaint();
}

void NamUiSourceSelector::setChannelActive(bool active)
{
    if (channelActive == active)
        return;
    channelActive = active;
    if (!channelActive && valueHovered)
    {
        valueHovered = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    repaint();
}

void NamUiSourceSelector::setOptions(juce::StringArray lines)
{
    if (deviceManager != nullptr)
        return;

    menuLines = std::move(lines);
    if (!menuLines.isEmpty())
        currentValue = menuLines[0];
    repaint();
}

void NamUiSourceSelector::setSelectedValue(const juce::String& value)
{
    currentValue = value;
    repaint();
}

void NamUiSourceSelector::attachToAudioDeviceManager(juce::AudioDeviceManager* manager)
{
    if (deviceManager == manager)
        return;

    if (deviceManager != nullptr)
        deviceManager->removeChangeListener(this);

    deviceManager = manager;

    if (deviceManager != nullptr)
        deviceManager->addChangeListener(this);

    if (deviceManager != nullptr)
    {
        rebuildFromDeviceManager();
        syncValueFromDeviceSetup();
    }

    repaint();
}

void NamUiSourceSelector::changeListenerCallback(juce::ChangeBroadcaster*)
{
    rebuildFromDeviceManager();
    syncValueFromDeviceSetup();
    if (menuLines.isEmpty() && valueHovered)
    {
        valueHovered = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    repaint();
}

void NamUiSourceSelector::rebuildFromDeviceManager()
{
    if (deviceManager == nullptr)
        return;

    menuLines.clear();
    menuOptions.clear();

    auto* type = deviceManager->getCurrentDeviceTypeObject();
    if (type == nullptr)
    {
        currentValue = "(No audio device)";
        repaint();
        return;
    }

    type->scanForDevices();

    auto setup = deviceManager->getAudioDeviceSetup();
    auto* currentDevice = deviceManager->getCurrentAudioDevice();
    const bool wantsInput = role == Role::input;
    auto deviceNames = type->getDeviceNames(wantsInput);

    auto addOption = [this](SourceOption option)
    {
        menuLines.add(option.label);
        menuOptions.add(std::move(option));
    };

    for (const auto& deviceName : deviceNames)
    {
        juce::StringArray channelNames;

        const bool isCurrentInput = wantsInput && setup.inputDeviceName == deviceName;
        const bool isCurrentOutput = !wantsInput && setup.outputDeviceName == deviceName;

        if (currentDevice != nullptr && (isCurrentInput || isCurrentOutput))
        {
            channelNames = wantsInput ? currentDevice->getInputChannelNames() : currentDevice->getOutputChannelNames();
        }
        else if (auto probe = std::unique_ptr<juce::AudioIODevice>(
                     type->createDevice(wantsInput ? juce::String{} : deviceName,
                                        wantsInput ? deviceName : juce::String{})))
        {
            channelNames = wantsInput ? probe->getInputChannelNames() : probe->getOutputChannelNames();
        }

        if (wantsInput)
        {
            for (int i = 0; i < channelNames.size(); ++i)
                addOption({ deviceName, makeInputLabel(deviceName, channelNames[i]), i, 1 });
        }
        else
        {
            for (int i = 0; i < channelNames.size(); i += 2)
                addOption({ deviceName, makeOutputLabel(deviceName, channelNames, i), i,
                            i + 1 < channelNames.size() ? 2 : 1 });
        }
    }

    if (menuLines.isEmpty())
    {
        currentValue = wantsInput ? "(No inputs)" : "(No outputs)";
        repaint();
        return;
    }

    syncValueFromDeviceSetup();
    repaint();
}

void NamUiSourceSelector::syncValueFromDeviceSetup()
{
    if (deviceManager == nullptr || menuOptions.isEmpty())
        return;

    auto setup = deviceManager->getAudioDeviceSetup();
    const auto deviceName = role == Role::input ? setup.inputDeviceName : setup.outputDeviceName;
    const auto bit = role == Role::input ? setup.inputChannels.findNextSetBit(0)
                                         : setup.outputChannels.findNextSetBit(0);

    for (const auto& option : menuOptions)
    {
        if (option.deviceName != deviceName)
            continue;

        if (bit >= option.channelStart && bit < option.channelStart + option.channelCount)
        {
            currentValue = option.label;
            return;
        }
    }
}

void NamUiSourceSelector::applyPick(int optionIndex)
{
    if (optionIndex < 0 || optionIndex >= menuLines.size())
        return;

    currentValue = menuLines.getReference(optionIndex);

    if (deviceManager == nullptr)
    {
        repaint();
        return;
    }

    if (optionIndex >= menuOptions.size())
        return;

    const auto option = menuOptions.getReference(optionIndex);
    auto setup = deviceManager->getAudioDeviceSetup();

    if (role == Role::input)
    {
        setup.inputDeviceName = option.deviceName;
        setup.useDefaultInputChannels = false;
        setup.inputChannels = {};
        setup.inputChannels.setBit(option.channelStart);
    }
    else
    {
        setup.outputDeviceName = option.deviceName;
        setup.useDefaultOutputChannels = false;
        setup.outputChannels = {};
        setup.outputChannels.setRange(option.channelStart, option.channelCount, true);
    }

    const auto error = deviceManager->setAudioDeviceSetup(setup, true);
    if (error.isNotEmpty())
        syncValueFromDeviceSetup();

    repaint();
}

void NamUiSourceSelector::resized()
{
    auto r = getLocalBounds();
    headerBounds = r.withHeight(26);
    valueArea = r.withTrimmedTop(headerBounds.getHeight() + kGapAfterHeader);
}

void NamUiSourceSelector::updateValueHover(const juce::Point<int>& pos)
{
    const bool over = !menuLines.isEmpty() && valueArea.contains(pos);
    if (over == valueHovered)
        return;
    valueHovered = over;
    repaint();
}

void NamUiSourceSelector::mouseMove(const juce::MouseEvent& e)
{
    updateValueHover(e.getPosition());
    setMouseCursor(valueHovered ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void NamUiSourceSelector::mouseExit(const juce::MouseEvent&)
{
    if (valueHovered)
    {
        valueHovered = false;
        repaint();
    }
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void NamUiSourceSelector::paint(juce::Graphics& g)
{
    const float corner = 2.f;

    // ── Header strip ──
    auto hb = headerBounds.toFloat().reduced(0.5f);
    g.setColour(tokens.borderDim);
    g.drawRoundedRectangle(hb, corner, 1.f);

    const float ledR = 3.f;
    const float gap = 7.f;
    auto headerInner = headerBounds.reduced(10, 4).toFloat();

    const auto ledColour = channelActive ? tokens.green : juce::Colours::transparentBlack;
    const auto ledBorder = tokens.textDim;

    auto paintLed = [&](float cx, float cy)
    {
        g.setColour(ledColour);
        g.fillEllipse(cx - ledR, cy - ledR, ledR * 2.f, ledR * 2.f);
        g.setColour(ledBorder);
        g.drawEllipse(cx - ledR, cy - ledR, ledR * 2.f, ledR * 2.f, 1.f);
    };

    auto paintSideLabel = [&](juce::Rectangle<float> textBounds, juce::Justification justification)
    {
        g.setColour(tokens.textMid);
        g.setFont(Fonts::michroma(11.f));
        g.drawText(sideLabel, textBounds, justification, false);
    };

    if (mirrored)
    {
        paintSideLabel(headerInner.withTrimmedRight(ledR * 2.f + gap), juce::Justification::centredRight);
        paintLed(headerInner.getRight() - ledR, headerInner.getCentreY());
    }
    else
    {
        paintLed(headerInner.getX() + ledR, headerInner.getCentreY());
        paintSideLabel(headerInner.withTrimmedLeft(ledR * 2.f + gap), juce::Justification::centredLeft);
    }

    // ── Value strip ──
    auto vb = valueArea.toFloat().reduced(0.5f);
    const bool menuReady = !menuLines.isEmpty();
    auto valueFill = tokens.bgPanel;
    if (valueHovered && menuReady)
        valueFill = valueFill.overlaidWith(tokens.amberGlow2);
    g.setColour(valueFill);
    g.fillRoundedRectangle(vb, corner);

    g.setColour(valueHovered && menuReady ? tokens.amber.withMultipliedAlpha(0.45f) : tokens.border);
    g.drawRoundedRectangle(vb, corner, 1.f);

    g.setColour(tokens.amber);
    g.setFont(Fonts::shareTechMono(12.f, 0.f));

    auto textJust = mirrored ? juce::Justification::centredRight : juce::Justification::centredLeft;
    auto textRect = valueArea.reduced(10, 5).toFloat();
    g.drawText(currentValue, textRect, textJust, true);
}

void NamUiSourceSelector::mouseDown(const juce::MouseEvent& e)
{
    if (!valueArea.contains(e.getPosition()))
        return;

    if (menuLines.isEmpty())
        return;

    juce::PopupMenu menu;
    menu.setLookAndFeel(&popupLookAndFeel);
    for (int i = 0; i < menuLines.size(); ++i)
        menu.addItem(i + 1, menuLines[i], true, menuLines[i] == currentValue);

    const auto anchorBottom = valueArea.withY(valueArea.getBottom()).withHeight(1);
    const auto selectedIndex = menuLines.indexOf(currentValue);
    auto* menuParent = getTopLevelComponent();

    menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetScreenArea(localAreaToGlobal(anchorBottom))
                           .withPreferredPopupDirection(juce::PopupMenu::Options::PopupDirection::downwards)
                           .withMinimumWidth(valueArea.getWidth())
                           .withMaximumNumColumns(1)
                           .withStandardItemHeight(valueArea.getHeight())
                           .withInitiallySelectedItem(selectedIndex >= 0 ? selectedIndex + 1 : 0)
                           .withParentComponent(menuParent != nullptr ? menuParent : this),
                       [this](int result)
        {
            if (result > 0)
                applyPick(result - 1);
        });
}

} // namespace NamUi
