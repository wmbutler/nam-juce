target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_sources(${PROJECT_NAME} PRIVATE
    NamUiBrowserRow.cpp
    NamUiBrowserRow.h
    NamUiSourceSelector.cpp
    NamUiSourceSelector.h
    NamUiColourPalette.h
    NamUiFonts.cpp
    NamUiFonts.h
    NamUiEditor.cpp
    NamUiEditor.h
    NamUiKnob.cpp
    NamUiKnob.h
    NamUiKnobGroup.cpp
    NamUiKnobGroup.h
    NamUiLed.cpp
    NamUiLed.h
    NamUiMainControlsStrip.cpp
    NamUiMainControlsStrip.h
    NamUiMetronomeStrip.cpp
    NamUiMetronomeStrip.h
    NamUiMuteButton.cpp
    NamUiMuteButton.h
    NamUiPresetPanel.cpp
    NamUiPresetPanel.h
    NamUiStandardButton.cpp
    NamUiStandardButton.h
)
