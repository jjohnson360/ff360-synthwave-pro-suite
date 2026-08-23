cmake_minimum_required(VERSION 3.20)
project(ff360_dsp_core VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
set(FF360_DSP_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Sources
set(FF360_DSP_SOURCES
    src/TapeEngine.cpp
    src/ModulationEngine.cpp
    src/ReverbEngine.cpp
    src/MacroSystem.cpp
    src/ParameterManager.cpp
    src/MeteringBridge.cpp
    src/StereoEngine.cpp
    src/TapeStopController.cpp
    src/GlitchEngine.cpp
    src/GenerativeEngine.cpp
    src/GranularTexture.cpp
    src/ff360_dsp_c_api.cpp
)

# Shared/Static library target
add_library(ff360_dsp_core STATIC ${FF360_DSP_SOURCES})

target_include_directories(ff360_dsp_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Test executables
enable_testing()
add_executable(ff360_dsp_core_tests tests/test_main.cpp)
target_link_libraries(ff360_dsp_core_tests PRIVATE ff360_dsp_core)
add_test(NAME CoreDspUnitTests COMMAND ff360_dsp_core_tests)

# Suite Benchmarks & Realtime Smoke Runner (Phase 1)
add_executable(ff360_suite_benchmarks tests/suite_benchmarks.cpp)
target_link_libraries(ff360_suite_benchmarks PRIVATE ff360_dsp_core)
target_include_directories(ff360_suite_benchmarks PRIVATE 
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../plugins
)

# Suite Benchmarks & Realtime Smoke Runner (Phase 2)
add_executable(ff360_suite_benchmarks_phase2 tests/suite_benchmarks_phase2.cpp)
target_link_libraries(ff360_suite_benchmarks_phase2 PRIVATE ff360_dsp_core)
target_include_directories(ff360_suite_benchmarks_phase2 PRIVATE 
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../plugins
)

# Collection Closeout Benchmarks (8 Plugins)
add_executable(ff360_suite_benchmarks_collection tests/suite_benchmarks_collection.cpp)
target_link_libraries(ff360_suite_benchmarks_collection PRIVATE ff360_dsp_core)
target_include_directories(ff360_suite_benchmarks_collection PRIVATE 
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../plugins
)
add_test(NAME SuiteBenchmarksPhase1 COMMAND ff360_suite_benchmarks)
add_test(NAME SuiteBenchmarksPhase2 COMMAND ff360_suite_benchmarks_phase2)
add_test(NAME SuiteBenchmarksCollection COMMAND ff360_suite_benchmarks_collection)
