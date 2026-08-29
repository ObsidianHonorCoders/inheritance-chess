## @file    configure_and_build.cmake
## @brief   Configure and build the project without running tests/executables.
## @details Decoupled build stage for local use and CI.

cmake_minimum_required(VERSION 3.28)

set(CMAKE_BUILD_TYPE "Release")
set(EXE_NAME "ichess_runner")
set(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(BUILD_DIR "${ROOT_DIR}/build")
option(CLEAN_BUILD "Remove build directory before configuring" ON)
option(BUILD_TESTS "Build centralized tests target during configure/build" ON)

if(CLEAN_BUILD AND EXISTS "${BUILD_DIR}")
    message(STATUS "Cleaning: Removing old build directory...")
    file(REMOVE_RECURSE "${BUILD_DIR}")
endif()

# Also clear FetchContent test sub-build cache to avoid generator/platform mismatches
# when switching toolchains or generators between runs.
if(CLEAN_BUILD AND EXISTS "${ROOT_DIR}/tests/build_gtest")
    message(STATUS "Cleaning: Removing tests/build_gtest cache directory...")
    file(REMOVE_RECURSE "${ROOT_DIR}/tests/build_gtest")
endif()

file(MAKE_DIRECTORY "${BUILD_DIR}")

include(${CMAKE_CURRENT_LIST_DIR}/detect_generator.cmake)

message(STATUS "Build Type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "Build Dir:     ${BUILD_DIR}")
message(STATUS "Executable:    ${EXE_NAME}")
message("")
message("======= CMake Configuration Phase =============================================")

set(CONFIG_COMMAND
    ${CMAKE_COMMAND}
    -S ${ROOT_DIR}
    -B ${BUILD_DIR}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DEXE_NAME=${EXE_NAME}
    -DENABLE_TESTS=${BUILD_TESTS}
)
if(NOT "${GENERATOR}" STREQUAL "")
    list(APPEND CONFIG_COMMAND -G "${GENERATOR}")
endif()

execute_process(
    COMMAND ${CONFIG_COMMAND}
    WORKING_DIRECTORY ${ROOT_DIR}
    RESULT_VARIABLE CONFIG_RESULT
    OUTPUT_VARIABLE CONFIG_OUTPUT
    ERROR_VARIABLE CONFIG_ERROR
)

if(NOT CONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR "CMAKE CONFIGURATION FAILED\n${CONFIG_OUTPUT}\n${CONFIG_ERROR}")
endif()
message(STATUS "Configuration completed successfully")

message("")
message("======= CMake Parallel Build Phase ============================================")

if(NOT NUM_CORES GREATER_EQUAL 1)
    set(NUM_CORES 1)
endif()

set(BUILD_COMMAND
    ${CMAKE_COMMAND}
    --build ${BUILD_DIR}
    --config ${CMAKE_BUILD_TYPE}
    --parallel ${NUM_CORES}
)
message(STATUS "Building with ${NUM_CORES} parallel cores...")

execute_process(
    COMMAND ${BUILD_COMMAND}
    WORKING_DIRECTORY ${ROOT_DIR}
    RESULT_VARIABLE BUILD_RESULT
    OUTPUT_VARIABLE BUILD_OUTPUT
    ERROR_VARIABLE BUILD_ERROR
)

if(NOT BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "BUILD FAILED with BUILD_RESULT ${BUILD_RESULT}\n${BUILD_OUTPUT}\n${BUILD_ERROR}")
endif()

message(STATUS "Build completed successfully")