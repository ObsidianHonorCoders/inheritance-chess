cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_definitions(-DUNICODE -D_UNICODE)

include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps/gtest")
FetchContent_Declare( googletest
                      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
                      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
                      EXCLUDE_FROM_ALL )
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# Explicit test source lists — do NOT use file(GLOB); add new test files here manually.
set(CCI_TEST_SOURCES
    ../chess_common_interface/tests/test_main.cpp
    ../chess_common_interface/tests/test_chess_types.cpp
    ../chess_common_interface/tests/test_grid.cpp
    ../chess_common_interface/tests/test_grid_adapted.cpp
    ../chess_common_interface/tests/test_move.cpp
    ../chess_common_interface/tests/test_piece_factory.cpp
    ../chess_common_interface/tests/test_position.cpp
)

set(CPL_TEST_SOURCES
    ../chess_pieces_logic/tests/test_bishop.cpp
    ../chess_pieces_logic/tests/test_chess_pawn.cpp
    ../chess_pieces_logic/tests/test_king.cpp
    ../chess_pieces_logic/tests/test_knight.cpp
    ../chess_pieces_logic/tests/test_pawn.cpp
    ../chess_pieces_logic/tests/test_pieces_logic.cpp
    ../chess_pieces_logic/tests/test_properties_integration.cpp
    ../chess_pieces_logic/tests/test_queen.cpp
    ../chess_pieces_logic/tests/test_rook.cpp
)

set(CTU_TEST_SOURCES
    ../chess_translation_unit/tests/test_fen_parsing.cpp
    ../chess_translation_unit/tests/test_translation_unit.cpp
)

set(CBM_TEST_SOURCES
    ../chess_board_manager/tests/test_board_manager.cpp
    ../chess_board_manager/tests/test_validation.cpp
)

set(CGH_TEST_SOURCES
    ../chess_game_handler/tests/test_game_handler.cpp
    ../chess_game_handler/tests/test_integration.cpp
    ../chess_game_handler/tests/test_performance.cpp
)

set(CES_TEST_SOURCES
    ../chess_event_system/tests/test_event_system.cpp
)

set(CEI_TEST_SOURCES
    ../chess_engine_interface/tests/test_chess_engine.cpp
)

add_executable("test_${EXE_NAME}"
    ${CCI_TEST_SOURCES}
    ${CPL_TEST_SOURCES}
    ${CTU_TEST_SOURCES}
    ${CBM_TEST_SOURCES}
    ${CGH_TEST_SOURCES}
    ${CES_TEST_SOURCES}
    ${CEI_TEST_SOURCES}
)

# Link with modular libraries
target_link_libraries("test_${EXE_NAME}" 
    gtest 
    gtest_main 
    gmock 
    cci
    cpl
    cbm
    ctu
    ces
    cgh
    cei
)

# Include directories for modular components
target_include_directories("test_${EXE_NAME}" PRIVATE 
    "${CMAKE_SOURCE_DIR}/chess_common_interface/include"
    "${CMAKE_SOURCE_DIR}/chess_pieces_logic/include"
    "${CMAKE_SOURCE_DIR}/chess_game_handler/include"
    "${CMAKE_SOURCE_DIR}/chess_board_manager/include"
    "${CMAKE_SOURCE_DIR}/chess_translation_unit/include"
    "${CMAKE_SOURCE_DIR}/chess_event_system/include"
    "${CMAKE_SOURCE_DIR}/chess_engine_interface/include"
)

gtest_discover_tests("test_${EXE_NAME}"
    DISCOVERY_MODE PRE_TEST
)
