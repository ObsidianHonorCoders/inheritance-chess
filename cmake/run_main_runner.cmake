## @file    run_main_runner.cmake
## @brief   Run ichess_runner from an existing build directory.
## @details Decoupled run stage (opt-in, local interactive usage).

cmake_minimum_required(VERSION 3.28)

set(CMAKE_BUILD_TYPE "Release")
set(EXE_NAME "ichess_runner")
set(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(BUILD_DIR "${ROOT_DIR}/build")

if(WIN32)
    set(EXE_PATH_CANDIDATE_1 "${BUILD_DIR}/${EXE_NAME}.exe")
    set(EXE_PATH_CANDIDATE_2 "${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${EXE_NAME}.exe")
else()
    set(EXE_PATH_CANDIDATE_1 "${BUILD_DIR}/${EXE_NAME}")
    set(EXE_PATH_CANDIDATE_2 "${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${EXE_NAME}")
endif()

if(EXISTS "${EXE_PATH_CANDIDATE_1}")
    set(RUN_EXE_PATH "${EXE_PATH_CANDIDATE_1}")
elseif(EXISTS "${EXE_PATH_CANDIDATE_2}")
    set(RUN_EXE_PATH "${EXE_PATH_CANDIDATE_2}")
else()
    message(FATAL_ERROR "Executable not found. Run cmake/configure_and_build.cmake first.")
endif()

message("======= Binary Executable Running Phase =======================================")
execute_process(COMMAND "${RUN_EXE_PATH}" RESULT_VARIABLE RUN_RESULT)
if(NOT RUN_RESULT EQUAL 0)
    message(WARNING "Application exited with code: ${RUN_RESULT}")
endif()
