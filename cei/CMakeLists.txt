# Chess Engine Interface (CEI) - AI and Search Algorithms

cmake_minimum_required(VERSION 3.20)

# Create CEI library
add_library(cei STATIC)

# Source files
target_sources(cei PRIVATE
    src/chess_engine.cpp
)

# Include directories
target_include_directories(cei PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Dependencies
target_link_libraries(cei PUBLIC 
    cci
    cpl
    cbm
    ctu
)

# Compiler-specific options
if(MSVC)
    target_compile_options(cei PRIVATE /W4)
else()
    target_compile_options(cei PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set C++ standard
set_property(TARGET cei PROPERTY CXX_STANDARD 20)
set_property(TARGET cei PROPERTY CXX_STANDARD_REQUIRED ON)

# Export symbols for Windows
if(WIN32)
    target_compile_definitions(cei PRIVATE ICHESS_CEI_EXPORTS)
endif()

# Optional: Build tests if requested
option(BUILD_CEI_TESTS "Build CEI tests" OFF)
if(BUILD_CEI_TESTS)
    # Enable testing
    enable_testing()
    
    # Find Google Test
    find_package(GTest REQUIRED)
    
    # Create test executable
    add_executable(cei_tests
        tests/test_chess_engine.cpp
    )
    
    # Link with libraries
    target_link_libraries(cei_tests
        PRIVATE cei
        PRIVATE GTest::gtest
        PRIVATE GTest::gtest_main
    )
    
    # Add test to CTest
    add_test(NAME CEI_Tests COMMAND cei_tests)
endif()
