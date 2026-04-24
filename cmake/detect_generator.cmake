## @file    detect_generator.cmake
## @brief   Identify the host system capabilities.
## @details Automated build system generator and host architecture detection.
##          Detects the host operating system and available build tools.
## 
## @section script_inputs  Inputs
## - None
##
## @section script_outputs Outputs (Variables set in parent scope or Cache)
## - DETECTED_PLATFORM:  The string name of the detected operating system.
## - DETECTED_ARCH:      The detected bit-depth of the host system.
## - NUM_CORES:			 The detected available processor cores amount.
## - GENERATOR:          The detected generator candidate.
##
## @section script_usage Usage
## include(detect_generator.cmake) 
## # OR
## cmake -P detect_generator.cmake
##
## @author  Calileus
## @version 1.0
## @date    2026-02-08

cmake_minimum_required(VERSION 3.28)

## @section platform_id Platform Identification
## @brief               Detects the operating system using CMake built-in variables
if(WIN32)
    set(DETECTED_PLATFORM "Windows")
elseif(APPLE)
    set(DETECTED_PLATFORM "macOS")
elseif(UNIX)
    set(DETECTED_PLATFORM "Linux")
else()
    set(DETECTED_PLATFORM "Unknown")
endif()

## @section arch_id System Architecture Detection  
## @brief           Determines the processor architecture of the host system
if(DEFINED ENV{PROCESSOR_ARCHITECTURE})
    set(DETECTED_ARCH "$ENV{PROCESSOR_ARCHITECTURE}")
else()
    set(DETECTED_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()

## @section cores_detect CPU Core Detection
## @brief                Counts the number of available processor cores using ProcessorCount module
##                       This value is used to enable parallel compilation with --parallel flag
include(ProcessorCount)
ProcessorCount(NUM_CORES)

## @section tool_discovery Tool Discovery
## @brief                  Searches for common build generators and compilers in system PATH

## @var   NINJA_PATH
## @brief Full path to ninja executable if found
find_program(NINJA_PATH ninja)

## @var   GCC_PATH
## @brief Full path to GCC compiler if found
find_program(GCC_PATH gcc)

## @var   MAKE_PATH
## @brief Full path to MinGW make executable if found
find_program(MAKE_PATH mingw32-make)

## @section generator_selection Generator Selection
## @brief                       Chooses the most appropriate build system generator
##                              Selection priority: MinGW Makefiles > Ninja > System Default
if(CMAKE_HOST_WIN32 AND GCC_PATH AND MAKE_PATH)
    set(GENERATOR "MinGW Makefiles")
elseif(NINJA_PATH)
    set(GENERATOR "Ninja")
else()
    set(GENERATOR "")
endif()

## @section summary Detection Summary
## @brief           Final report showing detected configuration.
message("")
message("======= Build Platform and System Architecture Detection ======================")
message(STATUS "Platform:      ${DETECTED_PLATFORM} (${DETECTED_ARCH})")
message(STATUS "Generator:     ${GENERATOR}")
message(STATUS "CPU Cores:     ${NUM_CORES}")
