/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/


#include <functional>

#include <juce_core/system/juce_TargetPlatform.h>

#include <juce_audio_plugin_client/detail/juce_CheckSettingMacros.h>
#include <juce_audio_plugin_client/detail/juce_IncludeSystemHeaders.h>
#include <juce_audio_plugin_client/detail/juce_IncludeModuleHeaders.h>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>

// You can set this flag in your build if you need to specify a different
// standalone JUCEApplication class for your app to use. If you don't
// set it then by default we'll just create a simple one as below.
#if JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP

    #include "AboutDialogComponent.h"
    #include "CustomStandaloneFilterWindow.h"

namespace juce
{

#if JUCE_MAC
void requestMacOSMicrophoneAccess(std::function<void()> completion);
#endif

//==============================================================================
class CustomStandaloneFilterApp : public JUCEApplication,
                                  private MenuBarModel
{
public:
    CustomStandaloneFilterApp()
    {
        PluginHostType::jucePlugInClientCurrentWrapperType = AudioProcessor::wrapperType_Standalone;

        PropertiesFile::Options options;

        options.applicationName = getApplicationName();
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
    #if JUCE_LINUX || JUCE_BSD
        options.folderName = "~/.config";
    #else
        options.folderName = "";
    #endif

        appProperties.setStorageParameters(options);
    }

    const String getApplicationName() override { return CharPointer_UTF8(JucePlugin_Name); }
    const String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void anotherInstanceStarted(const String&) override {}

    StringArray getMenuBarNames() override
    {
        return { "File" };
    }

    PopupMenu getMenuForIndex(int topLevelMenuIndex, const String&) override
    {
        PopupMenu menu;

        if (topLevelMenuIndex == 0)
        {
            menu.addItem(MenuItemIds::about, "About Neural Amp Modeler");
            menu.addItem(MenuItemIds::setToneDirectory, "Set Tone Directory...");
        }

        return menu;
    }

    void menuItemSelected(int menuItemID, int) override
    {
        switch (menuItemID)
        {
            case MenuItemIds::about:
            {
                DialogWindow::LaunchOptions options;
                options.dialogTitle = "About Neural Amp Modeler";
                options.dialogBackgroundColour = Colours::white;
                options.escapeKeyTriggersCloseButton = true;
                options.useNativeTitleBar = true;
                options.resizable = false;
                options.componentToCentreAround = mainWindow.get();
                options.content.setOwned(new AboutDialogComponent(getApplicationVersion()));
                options.launchAsync();
                break;
            }

            case MenuItemIds::setToneDirectory:
                showToneDirectoryChooser();
                break;

            default:
                break;
        }
    }

    virtual StandaloneFilterWindow* createWindow()
    {
    #ifdef JucePlugin_PreferredChannelConfigurations
        StandalonePluginHolder::PluginInOuts channels[] = {JucePlugin_PreferredChannelConfigurations};
    #endif

        return new StandaloneFilterWindow(getApplicationName(), LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId),
                                          appProperties.getUserSettings(), false, {}, nullptr
    #ifdef JucePlugin_PreferredChannelConfigurations
                                          ,
                                          juce::Array<StandalonePluginHolder::PluginInOuts>(channels, juce::numElementsInArray(channels))
    #else
                                          ,
                                          {}
    #endif
    #if JUCE_DONT_AUTO_OPEN_MIDI_DEVICES_ON_MOBILE
                                              ,
                                          false
    #endif
        );
    }

    //==============================================================================
    void initialise(const String&) override
    {
    #if JUCE_MAC
        MenuBarModel::setMacMainMenu(this);
    #endif

    #if JUCE_MAC
        requestMacOSMicrophoneAccess([this] { showMainWindow(); });
    #else
        showMainWindow();
    #endif
    }

    void showMainWindow()
    {
        mainWindow.reset(createWindow());

    #if JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE
        Desktop::getInstance().setKioskModeComponent(mainWindow.get(), false);
    #endif

        mainWindow->setVisible(true);
    }

    void shutdown() override
    {
    #if JUCE_MAC
        MenuBarModel::setMacMainMenu(nullptr);
    #endif

        toneDirectoryChooser = nullptr;
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        if (mainWindow.get() != nullptr)
            mainWindow->pluginHolder->savePluginState();

        if (ModalComponentManager::getInstance()->cancelAllModalComponents())
        {
            Timer::callAfterDelay(100,
                                  []()
                                  {
                                      if (auto app = JUCEApplicationBase::getInstance())
                                          app->systemRequestedQuit();
                                  });
        }
        else
        {
            quit();
        }
    }

protected:
    enum MenuItemIds
    {
        about = 1,
        setToneDirectory
    };

    File getInitialToneDirectoryChooserLocation()
    {
        if (auto* settings = appProperties.getUserSettings())
        {
            const File savedDirectory { settings->getValue("toneDirectory") };

            if (savedDirectory.isDirectory())
                return savedDirectory;
        }

        return File::getSpecialLocation(File::userHomeDirectory);
    }

    void showToneDirectoryChooser()
    {
        toneDirectoryChooser = std::make_unique<FileChooser>("Set Tone Directory",
                                                             getInitialToneDirectoryChooserLocation(),
                                                             String{},
                                                             true);

        toneDirectoryChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
                                          [this](const FileChooser& chooser)
        {
            const auto result = chooser.getResult();
            if (result == File{})
                return;

            const auto namRoot = resolveNamRootDirectory(result);
            const auto directoryResult = ensureNamDirectoryStructure(namRoot);

            if (directoryResult.failed())
            {
                AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Set Tone Directory",
                                                 "Unable to create the NAM directory structure:\n"
                                                     + directoryResult.getErrorMessage());
                return;
            }

            if (auto* settings = appProperties.getUserSettings())
            {
                settings->setValue("toneDirectory", namRoot.getFullPathName());
                appProperties.saveIfNeeded();
            }
        });
    }

    static File resolveNamRootDirectory(File selectedDirectory)
    {
        selectedDirectory = selectedDirectory.getFullPathName();

        for (auto current = selectedDirectory;; current = current.getParentDirectory())
        {
            if (current.getFileName().equalsIgnoreCase("NAM"))
                return current;

            const auto parent = current.getParentDirectory();

            if (parent == current)
                break;
        }

        return selectedDirectory.getChildFile("NAM");
    }

    static Result ensureNamDirectoryStructure(const File& namRoot)
    {
        if (auto result = namRoot.createDirectory(); result.failed())
            return result;

        for (const auto* childDirectoryName : { "Presets", "IRs", "Captures" })
            if (auto result = namRoot.getChildFile(childDirectoryName).createDirectory(); result.failed())
                return result;

        return Result::ok();
    }

    ApplicationProperties appProperties;
    std::unique_ptr<StandaloneFilterWindow> mainWindow;
    std::unique_ptr<FileChooser> toneDirectoryChooser;
};

} // namespace juce

    #if JucePlugin_Build_Standalone && JUCE_IOS

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wmissing-prototypes")

using namespace juce;

bool JUCE_CALLTYPE juce_isInterAppAudioConnected()
{
    if (auto holder = StandalonePluginHolder::getInstance())
        return holder->isInterAppAudioConnected();

    return false;
}

void JUCE_CALLTYPE juce_switchToHostApplication()
{
    if (auto holder = StandalonePluginHolder::getInstance())
        holder->switchToHostApplication();
}

Image JUCE_CALLTYPE juce_getIAAHostIcon(int size)
{
    if (auto holder = StandalonePluginHolder::getInstance())
        return holder->getIAAHostIcon(size);

    return Image();
}

JUCE_END_IGNORE_WARNINGS_GCC_LIKE

    #endif

#endif
