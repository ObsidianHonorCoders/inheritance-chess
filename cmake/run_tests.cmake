## @file    run_tests.cmake
## @brief   Run tests from an existing build directory.
## @details Decoupled test stage for local use and CI.

cmake_minimum_required(VERSION 3.28)

set(CMAKE_BUILD_TYPE "Release")
set(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(BUILD_DIR "${ROOT_DIR}/build")

if(NOT EXISTS "${BUILD_DIR}")
    message(FATAL_ERROR "Build directory not found: ${BUILD_DIR}. Run cmake/configure_and_build.cmake first.")
endif()

message("======= CTest Phase ============================================================")
execute_process(
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${BUILD_DIR} -C ${CMAKE_BUILD_TYPE} --output-on-failure --no-tests=error
    RESULT_VARIABLE TEST_RESULT
)

if(NOT TEST_RESULT EQUAL 0)
    message(FATAL_ERROR "TESTS FAILED with TEST_RESULT ${TEST_RESULT}")
endif()

message(STATUS "All tests passed")
