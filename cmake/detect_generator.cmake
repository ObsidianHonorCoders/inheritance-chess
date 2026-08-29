## @file    detect_generator.cmake
## @brief   Identify host system and choose a robust build generator.
## @details Detects platform/arch/cores and chooses a generator with practical
##          priority rules.
##
## @section platforms Supported Platforms & Generators
## | Platform | Default         | Fallback 1      | Fallback 2     |
## |----------|-----------------|-----------------|----------------|
## | Windows  | MinGW Makefiles | Ninja           | System Default |
## | Linux    | Ninja           | System Default  | -              |
## | macOS    | Ninja           | System Default  | -              |

cmake_minimum_required(VERSION 3.28)

if(WIN32)
    set(DETECTED_PLATFORM "Windows")
elseif(APPLE)
    set(DETECTED_PLATFORM "macOS")
elseif(UNIX)
    set(DETECTED_PLATFORM "Linux")
else()
    set(DETECTED_PLATFORM "Unknown")
endif()

if(DEFINED ENV{PROCESSOR_ARCHITECTURE} AND NOT "$ENV{PROCESSOR_ARCHITECTURE}" STREQUAL "")
    set(DETECTED_ARCH "$ENV{PROCESSOR_ARCHITECTURE}")
else()
    set(DETECTED_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()

include(ProcessorCount)
ProcessorCount(NUM_CORES)
if(NOT NUM_CORES GREATER_EQUAL 1)
    set(NUM_CORES 1)
endif()

set(PLATFORM "")

# Honor a user-selected generator if explicitly provided.
if(DEFINED CMAKE_GENERATOR AND NOT "${CMAKE_GENERATOR}" STREQUAL "")
    set(GENERATOR "${CMAKE_GENERATOR}")
else()
    # Follow generator preferences by platform
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        # Windows: MinGW Makefiles (default), Ninja (fallback), Unix Makefiles (system default)
        find_program(MINGW_PATH mingw32-make)
        if(MINGW_PATH)
            set(GENERATOR "MinGW Makefiles")
        else()
            find_program(NINJA_PATH ninja)
            if(NINJA_PATH)
                set(GENERATOR "Ninja")
            else()
                set(GENERATOR "Unix Makefiles")
            endif()
        endif()
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        # Linux: Ninja (default), Unix Makefiles (system default)
        find_program(NINJA_PATH ninja)
        if(NINJA_PATH)
            set(GENERATOR "Ninja")
        else()
            set(GENERATOR "Unix Makefiles")
        endif()
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        # macOS: Ninja (default), Unix Makefiles (system default)
        find_program(NINJA_PATH ninja)
        if(NINJA_PATH)
            set(GENERATOR "Ninja")
        else()
            set(GENERATOR "Unix Makefiles")
        endif()
    else()
        set(GENERATOR "")
    endif()
endif()

message("")
message("======= Build Platform and System Architecture Detection ======================")
message(STATUS "Platform:      ${DETECTED_PLATFORM} (${DETECTED_ARCH})")
message(STATUS "Generator:     ${GENERATOR}")
message(STATUS "CPU Cores:     ${NUM_CORES}")
