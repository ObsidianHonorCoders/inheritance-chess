cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_definitions(-DUNICODE -D_UNICODE)

project(inheritance_chess_project VERSION 0.0.0 LANGUAGES CXX)

# Single test ownership model: tests/CMakeLists.txt is the only place where
# test executables are defined.
option(ENABLE_TESTS "Build centralized test suite target" ON)

if(ENABLE_TESTS)
    enable_testing()
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add subdirectories for each component
add_subdirectory(chess_common_interface)
add_subdirectory(chess_pieces_logic)
add_subdirectory(chess_board_manager)
add_subdirectory(chess_translation_unit)
add_subdirectory(chess_event_system)
add_subdirectory(chess_game_handler)
add_subdirectory(chess_engine_interface)

# Main executable
add_executable(${EXE_NAME} chess_game_handler/src/ichess_runner.cpp)

# Link main executable with all component libraries
target_link_libraries(${EXE_NAME}
    cci
    cpl
    cbm
    ctu
    ces
    cgh
    cei
)

# Two-player console game (always built alongside the main executable)
add_executable(ichess_2p chess_game_handler/src/2p_game.cpp)
target_link_libraries(ichess_2p
    cci
    cpl
    cbm
    ctu
    ces
    cgh
    cei
)

# Add centralized tests
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
