cmake_minimum_required(VERSION 3.20)
project(ff360_synthwave_fx_suite VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core Shared DSP Library
add_subdirectory(ff360_dsp_core)

# Find or add JUCE
set(LOCAL_JUCE_PATH "C:/Users/fredd/OneDrive/Desktop/ff360_labs/Ecosystem Distortion/ff360lab_ecosytem_distortion/build/_deps/juce-src")
if(EXISTS "${LOCAL_JUCE_PATH}/CMakeLists.txt")
    message(STATUS "Found local JUCE at: ${LOCAL_JUCE_PATH}")
    add_subdirectory("${LOCAL_JUCE_PATH}" juce_build EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
    )
    FetchContent_MakeAvailable(juce)
endif()

function(add_synthwave_plugin TARGET_NAME PLUGIN_NAME PLUGIN_CODE)
    juce_add_plugin(${TARGET_NAME}
        COMPANY_NAME "ff360 Labs"
        BUNDLE_ID "com.ff360labs.${TARGET_NAME}"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE FF36
        PLUGIN_CODE ${PLUGIN_CODE}
        FORMATS VST3 AU Standalone
        PRODUCT_NAME "${PLUGIN_NAME}"
    )

    target_sources(${TARGET_NAME} PRIVATE ${ARGN})

    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ff360_dsp_core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/ff360_ui_core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
        ff360_dsp_core
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_graphics
        juce::juce_data_structures
        juce::juce_events
        juce::juce_core
    )

    target_compile_definitions(${TARGET_NAME} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )
endfunction()

# 1. VHS
add_synthwave_plugin(VHS "VHS" "Vhs0"
    plugins/vhs/PluginProcessor.cpp
    plugins/vhs/PluginProcessor.h
    plugins/vhs/PluginEditor.cpp
    plugins/vhs/PluginEditor.h
)

# 2. Neon Chorus
add_synthwave_plugin(NeonChorus "Neon Chorus" "NCh0"
    plugins/neon_chorus/PluginProcessor.cpp
    plugins/neon_chorus/PluginProcessor.h
    plugins/neon_chorus/PluginEditor.cpp
    plugins/neon_chorus/PluginEditor.h
)

# 3. Midnight Reverb
add_synthwave_plugin(MidnightReverb "Midnight Reverb" "MRev"
    plugins/midnight_reverb/PluginProcessor.cpp
    plugins/midnight_reverb/PluginProcessor.h
    plugins/midnight_reverb/PluginEditor.cpp
    plugins/midnight_reverb/PluginEditor.h
)

# 4. Neon Width
add_synthwave_plugin(NeonWidth "Neon Width" "NWdt"
    plugins/neon_width/PluginProcessor.cpp
    plugins/neon_width/PluginProcessor.h
    plugins/neon_width/PluginEditor.cpp
    plugins/neon_width/PluginEditor.h
)

# 5. Neon Tape Stop
add_synthwave_plugin(NeonTapeStop "Neon Tape Stop" "NTSt"
    plugins/neon_tape_stop/PluginProcessor.cpp
    plugins/neon_tape_stop/PluginProcessor.h
    plugins/neon_tape_stop/PluginEditor.cpp
    plugins/neon_tape_stop/PluginEditor.h
)

# 6. Cyberpunk Glitch
add_synthwave_plugin(CyberpunkGlitch "Cyberpunk Glitch" "CGlt"
    plugins/cyberpunk_glitch/PluginProcessor.cpp
    plugins/cyberpunk_glitch/PluginProcessor.h
    plugins/cyberpunk_glitch/PluginEditor.cpp
    plugins/cyberpunk_glitch/PluginEditor.h
)

# 7. RetroFX
add_synthwave_plugin(RetroFX "RetroFX" "RFX0"
    plugins/retrofx/PluginProcessor.cpp
    plugins/retrofx/PluginProcessor.h
    plugins/retrofx/PluginEditor.cpp
    plugins/retrofx/PluginEditor.h
)

# 8. NightDrive
add_synthwave_plugin(NightDrive "NightDrive" "NDrv"
    plugins/nightdrive/PluginProcessor.cpp
    plugins/nightdrive/PluginProcessor.h
    plugins/nightdrive/PluginEditor.cpp
    plugins/nightdrive/PluginEditor.h
)

message(STATUS "ff360_labs Synthwave FX Suite (All 8 Plugins + JUCE VST3 Targets) configured successfully.")
