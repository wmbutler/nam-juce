cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME "NEURAL_AMP_MODELER")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PLUGIN_VERSION LIMIT_COUNT 1)
string(STRIP "${PLUGIN_VERSION}" PLUGIN_VERSION)

# Must be set before project() so the toolchain targets arm64 on Apple Silicon.
# Use hardware sysctl (not uname): under Rosetta, uname -m may report x86_64.
execute_process(COMMAND uname -s OUTPUT_VARIABLE NAM_OSX_KERNEL_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NAM_OSX_KERNEL_NAME STREQUAL "Darwin")
    execute_process(COMMAND sysctl -n hw.optional.arm64 OUTPUT_VARIABLE NAM_HW_ARM64 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(NAM_HW_ARM64 STREQUAL "1")
        set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "macOS architectures" FORCE)
        message(STATUS "CMAKE_OSX_ARCHITECTURES: arm64 (Apple Silicon hardware)")
    endif()
endif()

project(${PROJECT_NAME} VERSION ${PLUGIN_VERSION})

# Release standalone → binaries/macos-arm64 (git-tracked). Only for arm64-native macOS configs.
set(NAM_PUBLISH_ARM_STANDALONE_TO_BINARIES FALSE)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(NAM_PUBLISH_ARM_STANDALONE_TO_BINARIES TRUE)
    elseif(NOT CMAKE_OSX_ARCHITECTURES AND NAM_HW_ARM64 STREQUAL "1")
        set(NAM_PUBLISH_ARM_STANDALONE_TO_BINARIES TRUE)
    endif()
endif()

set(NAM_GENERATED_INCLUDE_ROOT "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${NAM_GENERATED_INCLUDE_ROOT}/NAM")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Modules/NeuralAmpModelerCore/NAM/version.h.in"
    "${NAM_GENERATED_INCLUDE_ROOT}/NAM/version.h"
    @ONLY)

include("${CMAKE_CURRENT_SOURCE_DIR}/CMake/helpers.cmake")
init_submodules()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include_directories(SYSTEM /usr/local/include)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	link_libraries(stdc++fs)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    message(FATAL_ERROR "Unrecognized Platform!")
endif()

add_definitions(-DNAM_SAMPLE_FLOAT)
add_definitions(-DDSP_SAMPLE_FLOAT)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_PREFIX_PATH)
    find_package(JUCE CONFIG REQUIRED)
else()
    init_juce()
    add_subdirectory(Modules/JUCE)
endif()

include_directories(SYSTEM "${NAM_GENERATED_INCLUDE_ROOT}")
include_directories(SYSTEM Modules/NeuralAmpModelerCore)
include_directories(SYSTEM Modules/AudioDSPTools)
add_library(Eigen3::Eigen INTERFACE IMPORTED)
set_target_properties(Eigen3::Eigen PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/Modules/NeuralAmpModelerCore/Dependencies/eigen")
add_subdirectory(Modules/json)

juce_add_plugin(${PROJECT_NAME}
    VERSION ${PLUGIN_VERSION}
    ICON_BIG "Assets/ICON.png"
    ICON_SMALL "Assets/ICON.png"
    COMPANY_NAME "Manos Labrakis"
    BUNDLE_ID com.manoslabrakis.neuralampmodeler
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Neural Amp Modeler needs microphone access to process audio input."
    # IS_SYNTH TRUE/FALSE                 
    # NEEDS_MIDI_INPUT TRUE/FALSE           
    # NEEDS_MIDI_OUTPUT TRUE/FALSE        
    # IS_MIDI_EFFECT TRUE/FALSE         
    # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE
    # COPY_PLUGIN_AFTER_BUILD TRUE/FALSE  
    # PLUGIN_MANUFACTURER_CODE Juce             
    PLUGIN_CODE NMj0
    FORMATS AU VST3 Standalone                  
    PRODUCT_NAME "Neural Amp Modeler"
    JucePlugin_Manufacturer "Manos Labrakis"
    )        
    
juce_generate_juce_header(${PROJECT_NAME})

#add_subdirectory(Assets)

add_subdirectory(Assets)
add_subdirectory(Source)
include_directories(Source)

target_sources(${PROJECT_NAME}
PRIVATE
    Modules/ff_meters/ff_meters.h
    Modules/ff_meters/ff_meters.cpp
    Modules/NeuralAmpModelerCore/NAM/activations.cpp
    Modules/NeuralAmpModelerCore/NAM/activations.h
    Modules/NeuralAmpModelerCore/NAM/convnet.cpp
    Modules/NeuralAmpModelerCore/NAM/convnet.h
    Modules/NeuralAmpModelerCore/NAM/dsp.cpp
    Modules/NeuralAmpModelerCore/NAM/dsp.h
    Modules/NeuralAmpModelerCore/NAM/get_dsp.cpp
    Modules/NeuralAmpModelerCore/NAM/lstm.cpp
    Modules/NeuralAmpModelerCore/NAM/lstm.h
    Modules/NeuralAmpModelerCore/NAM/util.cpp
    Modules/NeuralAmpModelerCore/NAM/util.h
    "${NAM_GENERATED_INCLUDE_ROOT}/NAM/version.h"
    Modules/NeuralAmpModelerCore/NAM/wavenet.cpp
    Modules/NeuralAmpModelerCore/NAM/wavenet.h
    Modules/AudioDSPTools/dsp/dsp.cpp
    Modules/AudioDSPTools/dsp/dsp.h
    Modules/AudioDSPTools/dsp/ImpulseResponse.cpp
    Modules/AudioDSPTools/dsp/ImpulseResponse.h
    Modules/AudioDSPTools/dsp/NoiseGate.cpp
    Modules/AudioDSPTools/dsp/NoiseGate.h
    Modules/AudioDSPTools/dsp/RecursiveLinearFilter.cpp
    Modules/AudioDSPTools/dsp/RecursiveLinearFilter.h
    Modules/AudioDSPTools/dsp/Resample.h
    Modules/AudioDSPTools/dsp/ResamplingContainer
    Modules/AudioDSPTools/dsp/version.h
    Modules/AudioDSPTools/dsp/wav.cpp
    Modules/AudioDSPTools/dsp/wav.h    
)

set_source_files_properties("${NAM_GENERATED_INCLUDE_ROOT}/NAM/version.h" PROPERTIES GENERATED TRUE)

option(USE_NATIVE_ARCH "Enable architecture-specific optimizations (x86-64-v3 on x86_64 hosts only)" OFF)

if (USE_NATIVE_ARCH)
    if (APPLE AND CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        message(STATUS "USE_NATIVE_ARCH: skipping -march=x86-64-v3 (macOS arm64 build)")
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "(amd64)|(AMD64)|(x86_64)")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64-v3")
        message(STATUS "Enabling -march=x86-64-v3 for ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

if (BETA_RELEASE)
    set(PLUG_VERSION "${PLUGIN_VERSION} BETA")
else()
    set(PLUG_VERSION "${PLUGIN_VERSION}")
endif()

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        PLUG_VERSION="${PLUG_VERSION}"
    )     

if(ASIO_PATH)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_ASIO=1)
        message(STATUS "Set ASIO flag")
        target_include_directories(${PROJECT_NAME} PUBLIC ${ASIO_PATH})
        message(STATUS "\nAdded ASIO SDK to target include directories\n")
    else()
        message(STATUS "\nASIO not supported on this OS, ignoring the ASIO Flag...\n")
    endif()
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        BinaryData
        juce::juce_audio_utils
        juce::juce_dsp
        nlohmann_json::nlohmann_json
        Eigen3::Eigen
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "${NAM_GENERATED_INCLUDE_ROOT}/NAM"
        Modules/NeuralAmpModelerCore/NAM
        Modules/NeuralAmpModelerCore/Dependencies/eigen
        Modules/NeuralAmpModelerCore/Dependencies/nlohmann
        Modules/AudioDSPTools/dsp
        Modules/json/include
        Modules/json/include/nlohmann
        Modules/
    #INTERFACE
        #$<TARGET_PROPERTY:juce_plugin_modules,INCLUDE_DIRECTORIES>
    )

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_custom_command(TARGET ${PROJECT_NAME}_Standalone POST_BUILD
        COMMAND /usr/bin/codesign --force --sign - "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_Standalone>"
        COMMENT "Ad-hoc signing standalone app for macOS privacy permissions")

    if(NAM_PUBLISH_ARM_STANDALONE_TO_BINARIES)
        add_custom_command(TARGET ${PROJECT_NAME}_Standalone POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DNAM_BUILD_CONFIG=$<CONFIG>
                -DNAM_REPO_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
                -DNAM_BUNDLE=$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_Standalone>
                -P "${CMAKE_CURRENT_SOURCE_DIR}/CMake/CopyStandaloneToBinaries.cmake"
            COMMENT "Sync Release standalone to binaries/macos-arm64 (for commit)")
    endif()
endif()


if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(${PROJECT_NAME} 
    PRIVATE 
        NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if (MSVC)
	target_compile_options(${PROJECT_NAME} 
    PRIVATE
		"$<$<CONFIG:DEBUG>:/W4>"
		"$<$<CONFIG:RELEASE>:/O2>"
	)
else()
	target_compile_options(${PROJECT_NAME} 
    PRIVATE
		-Wall 
		"$<$<CONFIG:DEBUG>:-Og;-ggdb>"
		"$<$<CONFIG:RELWITHDEBINFO>:-Ofast>"
		"$<$<CONFIG:RELEASE>:-Ofast>"
	)
endif()
