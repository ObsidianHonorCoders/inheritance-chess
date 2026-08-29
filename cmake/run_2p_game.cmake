## @file    run_2p_game.cmake
## @brief   Run ichess_2p from an existing build directory.
## @details Decoupled two-player interactive run stage (opt-in).

cmake_minimum_required(VERSION 3.28)

set(CMAKE_BUILD_TYPE "Release")
set(GAME_EXE_NAME "ichess_2p")
set(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(BUILD_DIR "${ROOT_DIR}/build")

if(WIN32)
    set(GAME_PATH_CANDIDATE_1 "${BUILD_DIR}/${GAME_EXE_NAME}.exe")
    set(GAME_PATH_CANDIDATE_2 "${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${GAME_EXE_NAME}.exe")
else()
    set(GAME_PATH_CANDIDATE_1 "${BUILD_DIR}/${GAME_EXE_NAME}")
    set(GAME_PATH_CANDIDATE_2 "${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${GAME_EXE_NAME}")
endif()

if(EXISTS "${GAME_PATH_CANDIDATE_1}")
    set(RUN_GAME_PATH "${GAME_PATH_CANDIDATE_1}")
elseif(EXISTS "${GAME_PATH_CANDIDATE_2}")
    set(RUN_GAME_PATH "${GAME_PATH_CANDIDATE_2}")
else()
    message(FATAL_ERROR "Two-player executable not found. Run cmake/configure_and_build.cmake first.")
endif()

message("======= Launching Two-Player Console Game =====================================")
execute_process(COMMAND "${RUN_GAME_PATH}" RESULT_VARIABLE GAME_RESULT)
if(NOT GAME_RESULT EQUAL 0)
    message(WARNING "Game exited with code: ${GAME_RESULT}")
endif()
