cmake_minimum_required(VERSION 3.20)
project(ff360_synthwave_fx_suite VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core Shared DSP Library
add_subdirectory(ff360_dsp_core)

message(STATUS "ff360_labs Synthwave FX Suite (Phase 1: Core FX) configured successfully.")
