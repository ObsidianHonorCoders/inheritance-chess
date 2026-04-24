# Chess Common Interface (CCI) - Core Types and Data Structures

cmake_minimum_required(VERSION 3.20)

# Create CCI library
add_library(cci STATIC)

# Source files
target_sources(cci PRIVATE
    src/chess_types.cpp
    src/grid.cpp
    src/move.cpp
    src/position.cpp
)

# Include directories
target_include_directories(cci PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Compiler-specific options
if(MSVC)
    target_compile_options(cci PRIVATE /W4)
else()
    target_compile_options(cci PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set C++ standard
set_property(TARGET cci PROPERTY CXX_STANDARD 20)
set_property(TARGET cci PROPERTY CXX_STANDARD_REQUIRED ON)

# Export symbols for Windows
if(WIN32)
    target_compile_definitions(cci PRIVATE ICHESS_CCI_EXPORTS)
endif()

# Optional: Build tests if requested
option(BUILD_CCI_TESTS "Build CCI tests" OFF)
if(BUILD_CCI_TESTS)
    # Enable testing
    enable_testing()
    
    # Find Google Test
    find_package(GTest REQUIRED)
    
    # Create test executable
    add_executable(cci_tests
        tests/test_chess_types.cpp
        tests/test_grid.cpp
        tests/test_grid_adapted.cpp
        tests/test_move.cpp
        tests/test_piece_factory.cpp
        tests/test_position.cpp
        tests/test_main.cpp
    )
    
    # Link with libraries
    target_link_libraries(cci_tests
        PRIVATE cci
        PRIVATE GTest::gtest
        PRIVATE GTest::gtest_main
    )
    
    # Add test to CTest
    add_test(NAME CCI_Tests COMMAND cci_tests)
endif()
