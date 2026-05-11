target_sources(${PROJECT_NAME}
PRIVATE
    ResamplingNAM.h
    NeuralAmpModeler.cpp
    NeuralAmpModeler.h
    StatusedTrigger.cpp
    StatusedTrigger.h
    ToneStack.cpp
    ToneStack.h
    PluginEditor.cpp
    PluginEditor.h
    PluginProcessor.cpp
    PluginProcessor.h
    EqProcessor.cpp
    EqProcessor.h
    DoublerProcessor.h
    architecture.hpp
)

add_subdirectory(NamUi)

add_subdirectory(CustomStandaloneWindow)
include_directories(CustomStandaloneWindow)

add_subdirectory(PresetManager)
include_directories(PresetManager)
