################################################################################
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
## | Platform | Default | Fallback 1      | Fallback 2     |
## |----------|---------|-----------------|----------------|
## | Windows  | Ninja   | MinGW Makefiles | System Default |
## | Linux    | Ninja   | System Default  | -              |
## | macOS    | Ninja   | System Default  | -              |
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
## - Selecting the fastest available build generator (Ninja preferred)
## - Using Release build configuration for optimized binaries
## - Displaying compilation progress in real-time
##
## @author Build System
## @version 1.0
## @date 2026
################################################################################

cmake_minimum_required(VERSION 3.28)

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

include(cmakehelpers/detect_generator.cmake)

message(STATUS "Build Type:    Release (optimized build)")
message(STATUS "Build Dir:     ${BUILD_DIR}/")
message(STATUS "Executable:    ${EXE_PATH}")

################################################################################
## @name CMake Configuration Phase
## @brief Configure the project build system from CMakeLists.txt
################################################################################
message("================================ CONFIGURATION PHASE ==========================")

## @section config CMake Configuration
## @brief Runs cmake to generate platform-specific build files

## @var CONF_ARGS
## @brief Arguments passed to cmake configuration
## Includes: source directory (-S .), build directory (-B), build type, executable name

set(CONF_ARGS -S . -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=Release -DEXE_NAME=${EXE_NAME})

## @brief Append generator argument if one was automatically selected
if(NOT "${GENERATOR}" STREQUAL "")
    list(APPEND CONF_ARGS -G "${GENERATOR}")
endif()

## @brief Execute CMake configuration
execute_process(
    COMMAND ${CMAKE_COMMAND} ${CONF_ARGS} 
    RESULT_VARIABLE CONFIG_RESULT
)

## @brief Check configuration result and abort on failure with helpful diagnostics
if(NOT CONFIG_RESULT EQUAL 0) 
    message(FATAL_ERROR "CMAKE CONFIGURATION FAILED
    
This likely means:
- Your C/C++ compiler is not in the system PATH
- CMake could not find the compiler for the selected generator
- There's an error in your CMakeLists.txt file

Please ensure you have a compatible compiler installed:
- Windows: Visual Studio, GCC (MinGW), or Clang
- Linux: GCC or Clang
- macOS: Xcode Command Line Tools (clang)") 
endif()

message(STATUS "Configuration completed successfully")

################################################################################
## @name CMake Build Phase
## @brief Compile the project using all available CPU cores
################################################################################

message("================================ BUILD PHASE ==================================")

## @section build_exec Build Execution with Parallel Compilation
## @brief Compiles the project using all detected CPU cores
## The --parallel flag is critical for utilizing multi-core systems

## @brief Validate core count and ensure minimum of 1
if(NOT NUM_CORES GREATER_EQUAL 1)
    set(NUM_CORES 1)
endif()

## @var   BUILD_COMMAND_ARGS
## @brief Arguments for cmake --build command
set(BUILD_COMMAND_ARGS --build ${BUILD_DIR} --config Release --parallel ${NUM_CORES})

## @brief Execute the build with parallel compilation across all CPU cores
## @note Build progress and compiler output is displayed in real-time
message(STATUS "Building with ${NUM_CORES} parallel cores...")

execute_process(
    COMMAND ${CMAKE_COMMAND} ${BUILD_COMMAND_ARGS}
    RESULT_VARIABLE BUILD_RESULT
)

## @brief Check build result and abort on failure with helpful diagnostics
if(NOT BUILD_RESULT EQUAL 0) 
    message(FATAL_ERROR "BUILD FAILED

The compilation encountered errors. Check the output above for details.
Common issues:
- Missing header files or dependencies
- Syntax errors in source code
- Incompatible compiler flags or settings") 
endif()

message(STATUS "Build completed successfully using all ${NUM_CORES} CPU cores")

################################################################################
## @name Executable Location and Verification
## @brief Determine expected executable location based on platform
################################################################################

message("================================ EXECUTION PHASE ==============================")


message(STATUS "Looking for executable at: ${EXE_PATH}")

## @brief Verify executable exists before attempting execution
if(EXISTS ${EXE_PATH})
    message(STATUS "Executable found, launching...")
    message("================================================")
    
    ## @section exec_run Program Execution
    ## @brief Runs the compiled executable and captures exit code
    execute_process(
        COMMAND ${EXE_PATH}
        RESULT_VARIABLE RUN_RESULT
    )
    
    message("================================================")
    
    ## @brief Report execution status
    if(RUN_RESULT EQUAL 0)
        message(STATUS "Application executed successfully (exit code: 0)")
    else()
        message(WARNING "Application exited with code: ${RUN_RESULT}")
    endif()
else()
    message(FATAL_ERROR "EXECUTABLE NOT FOUND at: ${EXE_PATH}

This usually means:
- The build failed silently (check build output)
- The executable name is incorrect (check EXE_NAME variable)
- The project's CMakeLists.txt does not create an executable target")
endif()
