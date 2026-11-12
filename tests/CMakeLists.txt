cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_definitions(-DUNICODE -D_UNICODE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_compile_options(-Wno-error=character-conversion)
endif()

include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps/gtest")
FetchContent_Declare( googletest
                      URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
                      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
                      EXCLUDE_FROM_ALL )
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# Explicit test source lists — do NOT use file(GLOB); add new test files here manually.
set(CCI_TEST_SOURCES
    ../domain/core/chess_common_interface/tests/test_main.cpp
    ../domain/core/chess_common_interface/tests/test_chess_types.cpp
    ../domain/core/chess_common_interface/tests/test_grid.cpp
    ../domain/core/chess_common_interface/tests/test_grid_adapted.cpp
    ../domain/core/chess_common_interface/tests/test_move.cpp
    ../domain/core/chess_common_interface/tests/test_piece_factory.cpp
    ../domain/core/chess_common_interface/tests/test_position.cpp
)

set(CPL_TEST_SOURCES
    ../domain/core/chess_pieces_logic/tests/test_bishop.cpp
    ../domain/core/chess_pieces_logic/tests/test_chess_pawn.cpp
    ../domain/core/chess_pieces_logic/tests/test_king.cpp
    ../domain/core/chess_pieces_logic/tests/test_knight.cpp
    ../domain/core/chess_pieces_logic/tests/test_pawn.cpp
    ../domain/core/chess_pieces_logic/tests/test_pieces_logic.cpp
    ../domain/core/chess_pieces_logic/tests/test_properties_integration.cpp
    ../domain/core/chess_pieces_logic/tests/test_queen.cpp
    ../domain/core/chess_pieces_logic/tests/test_rook.cpp
)

set(CTU_TEST_SOURCES
    ../domain/core/chess_translation_unit/tests/test_fen_parsing.cpp
    ../domain/core/chess_translation_unit/tests/test_translation_unit.cpp
)

set(CBM_TEST_SOURCES
    ../domain/core/chess_board_manager/tests/test_board_manager.cpp
    ../domain/core/chess_board_manager/tests/test_validation.cpp
)

set(CGH_TEST_SOURCES
    ../domain/core/chess_game_handler/tests/test_game_handler.cpp
    ../domain/core/chess_game_handler/tests/test_integration.cpp
    ../domain/core/chess_game_handler/tests/test_performance.cpp
)

set(CES_TEST_SOURCES
    ../domain/core/chess_event_system/tests/test_event_system.cpp
)

set(CEI_TEST_SOURCES
    ../domain/core/chess_engine_interface/tests/test_chess_engine.cpp
)

set(BACKEND_TEST_SOURCES)
set(BACKEND_TEST_LIBS)
if(ICHESS_BUILD_BACKEND)
    set(BACKEND_TEST_SOURCES
        ../backend/tests/test_http_api.cpp
        ../backend/tests/test_sqlite_database.cpp
        ../backend/tests/test_sqlite_game_operations.cpp
        ../backend/tests/test_http_validation.cpp
        ../backend/tests/test_session_authenticator.cpp
        ../backend/tests/test_local_account_store.cpp
        ../backend/tests/test_session_revocation_store.cpp
        ../backend/tests/test_service_runtime.cpp
        ../backend/tests/test_execution_budget.cpp
        ../backend/tests/test_observability.cpp
    )
    set(BACKEND_TEST_LIBS ichess_backend)
endif()

add_executable("test_${EXE_NAME}"
    ${CCI_TEST_SOURCES}
    ${CPL_TEST_SOURCES}
    ${CTU_TEST_SOURCES}
    ${CBM_TEST_SOURCES}
    ${CGH_TEST_SOURCES}
    ${CES_TEST_SOURCES}
    ${CEI_TEST_SOURCES}
    ${BACKEND_TEST_SOURCES}
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
    ${BACKEND_TEST_LIBS}
)

# Include directories for modular components
target_include_directories("test_${EXE_NAME}" PRIVATE 
    "${CMAKE_SOURCE_DIR}/domain/core/chess_common_interface/include"
    "${CMAKE_SOURCE_DIR}/domain/core/chess_pieces_logic/include"
    "${CMAKE_SOURCE_DIR}/domain/core/chess_game_handler/include"
    "${CMAKE_SOURCE_DIR}/domain/core/chess_board_manager/include"
    "${CMAKE_SOURCE_DIR}/domain/core/chess_translation_unit/include"
    "${CMAKE_SOURCE_DIR}/domain/core/chess_event_system/include"
    "${CMAKE_SOURCE_DIR}/domain/core/chess_engine_interface/include"
)

gtest_discover_tests("test_${EXE_NAME}"
    DISCOVERY_MODE PRE_TEST
)
