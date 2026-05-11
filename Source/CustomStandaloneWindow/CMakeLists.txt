target_sources(${PROJECT_NAME}
PRIVATE
    AboutDialogComponent.cpp
    AboutDialogComponent.h
    CustomStandaloneClient.cpp
    CustomStandaloneFilterApp.cpp
    CustomStandaloneFilterWindow.h
)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_sources(${PROJECT_NAME} PRIVATE MacMicrophonePermission.mm)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework AVFoundation")
endif()
