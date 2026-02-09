## @file    build_and_run_project.cmake
## @brief   Cross-platform CMake build and execution script
## @details Comprehensive build automation script that configures, compiles, and runs
##          a CMake-based C++ project across multiple platforms (Windows, Linux, macOS).
##          The script automatically inform the host platform, available compilers,
##          and detect system resources (CPU cores) to optimize the build process.
##
## @section workflow Workflow
## -# Calls detect_generator.cmake to get:
##    - DETECTED_PLATFORM: The string name of the detected operating system.
##    - DETECTED_ARCH:     The detected bit-depth of the host system.
##    - NUM_CORES:         The detected available processor cores amount.
##    - GENERATOR:         The detected generator candidate.
## -# Configures the project with CMake (Release build only)
## -# Builds the project using all available CPU cores in parallel
## -# Runs the compiled executable
## -# Reports build status and execution results
##
## @section features Features
## - **Cross-platform Support**:   Works seamlessly across different operating systems
## - **Generator Selection**:      Chooses available build system (more generators may be added)
## - **Parallel Compilation**:     Uses all system CPU cores to maximize build speed
## - **Error Handling**:           Graceful error reporting at each build stage
## - **Automatic Execution Test**: Runs platform-specific compiled binary after successful build
##
## @section platforms Supported Platforms & Generators
## | Platform | Default         | Fallback 1      | Fallback 2     |
## |----------|-----------------|-----------------|----------------|
## | Windows  | MinGW Makefiles | Ninja           | System Default |
## | Linux    | Ninja           | System Default  | -              |
## | macOS    | Ninja           | System Default  | -              |
##
## @section usage Usage
## @code
##   cmake -P build_and_run_project.cmake
## @endcode
##
## @section configuration Configuration
## Edit the following variables at the beginning of the script to customize:
## - `EXE_NAME`:  Name of the executable to build      (default: ichess_runner)
## - `BUILD_DIR`: Output directory for build artifacts (default: build)
##
## @section requirements Requirements
## - CMake 3.15 or higher
## - A C/C++ compiler (GCC, Clang, MSVC, etc.)
## - At least one build system generator available
##
## @section performance Performance Optimization
## This script maximizes build performance by:
## - Utilizing all available CPU cores for parallel compilation
## - Using Release build configuration for optimized binaries
## - Displaying compilation progress in real-time
##
## @author  Calileus
## @version 1.0
## @date    2026-02-08

cmake_minimum_required(VERSION 3.28)
set(CMAKE_BUILD_TYPE "Release")

## @var     EXE_NAME
## @brief   Name of the executable to build
## @details This is used as config parameter
set(EXE_NAME "ichess_runner")

## @var     BUILD_DIR
## @brief   Output directory for build artifacts
## @details All build files and the executable will be placed here
set(BUILD_DIR "build")

## @section exe_location Executable Path Resolution
## @brief                Constructs the path to the compiled executable
##                       Accounts for platform differences: .exe on Windows, bare name on Unix
if(WIN32)
    set(EXE_PATH "${BUILD_DIR}/${EXE_NAME}.exe")
else()
    set(EXE_PATH "${BUILD_DIR}/${EXE_NAME}")
endif()

## @section folderclean CMake Previous Build folder cleaning
## @brief               Erase the build folder if it exists
if(EXISTS "${BUILD_DIR}")
    message(STATUS "Cleaning: Removing old build directory...")
    file(REMOVE_RECURSE "${BUILD_DIR}")
endif()
file(MAKE_DIRECTORY "${BUILD_DIR}")

## @section detect_generator Generator Detection
## -# Calls detect_generator.cmake to get:
##    - DETECTED_PLATFORM: The string name of the detected operating system.
##    - DETECTED_ARCH:     The detected bit-depth of the host system.
##    - NUM_CORES:         The detected available processor cores amount.
##    - GENERATOR:         The detected generator candidate.
include(cmakehelpers/detect_generator.cmake)

## @section config CMake Configuration
## @brief          Runs cmake to generate platform-specific build files
message(STATUS "Build Type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "Build Dir:     ${BUILD_DIR}/")
message(STATUS "Executable:    ${EXE_PATH}")
message("======= CMake Configuration Phase =============================================")

## @var   CONF_ARGS
## @brief Arguments passed to cmake configuration
##        Includes: build directory (-B), build type, executable name
##        Append: generator argument if one was automatically selected
set(CONF_ARGS -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DEXE_NAME=${EXE_NAME})
if(NOT "${GENERATOR}" STREQUAL "")
    list(APPEND CONF_ARGS -G "${GENERATOR}")
endif()

## @brief Execute CMake configuration
execute_process( COMMAND ${CMAKE_COMMAND} ${CONF_ARGS} RESULT_VARIABLE CONFIG_RESULT)

## @brief Check configuration result and abort on failure with helpful diagnostics
if(NOT CONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR
"CMAKE CONFIGURATION FAILED
 This likely means:
 - Your C/C++ compiler is not in the system PATH
 - CMake could not find the compiler for the selected generator
 - There's an error in your CMakeLists.txt file
 Please ensure you have a compatible working compiler installed:
 - Windows: Visual Studio, GCC (MinGW), or Clang
 - Linux: GCC or Clang
 - macOS: Xcode Command Line Tools (clang)")
endif()
message(STATUS "Configuration completed successfully")

## @section build_exec Build with Parallel Compilation
## @brief              Compiles the project using all detected CPU cores
##                     The --parallel flag is critical for utilizing multi-core systems
message("======= CMake Parallel Build Phase ============================================")

## @brief Validate core count and ensure minimum of 1
if(NOT NUM_CORES GREATER_EQUAL 1)
    set(NUM_CORES 1)
endif()

## @var   BUILD_COMMAND_ARGS
## @brief Arguments for cmake --build command
set(BUILD_COMMAND_ARGS --build ${BUILD_DIR} --config ${CMAKE_BUILD_TYPE} --parallel ${NUM_CORES})
message(STATUS "Building with ${NUM_CORES} parallel cores...")

## @brief Execute the build with parallel compilation across all CPU cores
## @note  Build progress and compiler output is displayed in real-time
execute_process( COMMAND ${CMAKE_COMMAND} ${BUILD_COMMAND_ARGS} RESULT_VARIABLE BUILD_RESULT)

## @brief Check build result and abort on failure
if(NOT BUILD_RESULT EQUAL 0) 
    message(FATAL_ERROR "BUILD FAILED with BUILD_RESULT ${BUILD_RESULT}")
endif()
message(STATUS "Build completed successfully using all ${NUM_CORES} CPU cores")

## @section bin_exec Executable Location and Verification
## @brief            Verify expected executable location and run it
message("======= Binary Executable Running Phase =======================================")
message(STATUS "Looking for executable at: ${EXE_PATH}")

## @brief Verify executable exists before attempting execution
if(EXISTS ${EXE_PATH})
    message(STATUS "Executable found, launching...")
    message("===============================================================================")
    ## @section exec_run Program Execution
    ## @brief Runs the compiled executable and captures exit code
    execute_process( COMMAND ${EXE_PATH} RESULT_VARIABLE RUN_RESULT)
    message("===============================================================================")
    
    ## @brief Report execution status
    if(RUN_RESULT EQUAL 0)
        message(STATUS "Application executed successfully (exit code: 0)")
    else()
        message(WARNING "Application exited with code: ${RUN_RESULT}")
    endif()
else()
    message(FATAL_ERROR "EXECUTABLE NOT FOUND at: ${EXE_PATH}")
endif()

message("Thanks for using build_and_run_project.cmake script!")
message("")
