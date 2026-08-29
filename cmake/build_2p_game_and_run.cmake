## @file    build_2p_game_and_run.cmake
## @brief   Legacy compatibility wrapper for build + test + ichess_2p launch.
## @details Thin wrapper over decoupled scripts in this same folder.

cmake_minimum_required(VERSION 3.28)

option(BUILD_TESTS "Build and run tests" ON)
option(RUN_GAME "Run ichess_2p after build/test" ON)
option(CLEAN_BUILD "Remove build directory before configuring" ON)

message("======= Legacy Wrapper: build_2p_game_and_run.cmake ===========================")

execute_process(
    COMMAND ${CMAKE_COMMAND} -DCLEAN_BUILD=${CLEAN_BUILD} -DBUILD_TESTS=${BUILD_TESTS} -P "${CMAKE_CURRENT_LIST_DIR}/configure_and_build.cmake"
    RESULT_VARIABLE BUILD_STAGE_RESULT
)
if(NOT BUILD_STAGE_RESULT EQUAL 0)
    message(FATAL_ERROR "Build stage failed with code: ${BUILD_STAGE_RESULT}")
endif()

if(BUILD_TESTS)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_LIST_DIR}/run_tests.cmake"
        RESULT_VARIABLE TEST_STAGE_RESULT
    )
    if(NOT TEST_STAGE_RESULT EQUAL 0)
        message(FATAL_ERROR "Test stage failed with code: ${TEST_STAGE_RESULT}")
    endif()
else()
    message(STATUS "BUILD_TESTS=OFF, skipping test stage")
endif()

if(RUN_GAME)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_LIST_DIR}/run_2p_game.cmake"
        RESULT_VARIABLE RUN_STAGE_RESULT
    )
    if(NOT RUN_STAGE_RESULT EQUAL 0)
        message(WARNING "Run stage exited with code: ${RUN_STAGE_RESULT}")
    endif()
else()
    message(STATUS "RUN_GAME=OFF, skipping run stage")
endif()

message("Legacy wrapper completed.")
