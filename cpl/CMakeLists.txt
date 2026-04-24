# Chess Pieces Logic (CPL) - Individual Piece Movement Rules

cmake_minimum_required(VERSION 3.20)

# Create CPL library
add_library(cpl STATIC)

# Source files
target_sources(cpl PRIVATE
    src/chess_bishop.cpp
    src/chess_king.cpp
    src/chess_knight.cpp
    src/chess_pawn.cpp
    src/chess_queen.cpp
    src/chess_rook.cpp
    src/pieces_logic.cpp
    src/piece_factory.cpp
)

# Include directories
target_include_directories(cpl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Dependencies
target_link_libraries(cpl PUBLIC cci)

# Compiler-specific options
if(MSVC)
    target_compile_options(cpl PRIVATE /W4)
else()
    target_compile_options(cpl PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set C++ standard
set_property(TARGET cpl PROPERTY CXX_STANDARD 20)
set_property(TARGET cpl PROPERTY CXX_STANDARD_REQUIRED ON)

# Export symbols for Windows
if(WIN32)
    target_compile_definitions(cpl PRIVATE ICHESS_CPL_EXPORTS)
endif()

# Optional: Build tests if requested
option(BUILD_CPL_TESTS "Build CPL tests" OFF)
if(BUILD_CPL_TESTS)
    # Enable testing
    enable_testing()
    
    # Find Google Test
    find_package(GTest REQUIRED)
    
    # Create test executable
    add_executable(cpl_tests
        tests/test_bishop.cpp
        tests/test_chess_pawn.cpp
        tests/test_king.cpp
        tests/test_knight.cpp
        tests/test_pawn.cpp
        tests/test_pieces_logic.cpp
        tests/test_properties_integration.cpp
        tests/test_queen.cpp
        tests/test_rook.cpp
    )
    
    # Link with libraries
    target_link_libraries(cpl_tests
        PRIVATE cpl
        PRIVATE GTest::gtest
        PRIVATE GTest::gtest_main
    )
    
    # Add test to CTest
    add_test(NAME CPL_Tests COMMAND cpl_tests)
endif()
