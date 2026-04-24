# Chess Game Handler (CGH) - Game Orchestration and Lifecycle

cmake_minimum_required(VERSION 3.20)

# Create CGH library
add_library(cgh STATIC)

# Source files
target_sources(cgh PRIVATE
    src/game_handler.cpp
)

# Include directories
target_include_directories(cgh PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Dependencies
target_link_libraries(cgh PUBLIC 
    cci
    cpl
    cbm
    ctu
    ces
)

# Compiler-specific options
if(MSVC)
    target_compile_options(cgh PRIVATE /W4)
else()
    target_compile_options(cgh PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set C++ standard
set_property(TARGET cgh PROPERTY CXX_STANDARD 20)
set_property(TARGET cgh PROPERTY CXX_STANDARD_REQUIRED ON)

# Export symbols for Windows
if(WIN32)
    target_compile_definitions(cgh PRIVATE ICHESS_CGH_EXPORTS)
endif()

# Optional: Build tests if requested
option(BUILD_CGH_TESTS "Build CGH tests" OFF)
if(BUILD_CGH_TESTS)
    # Enable testing
    enable_testing()
    
    # Find Google Test
    find_package(GTest REQUIRED)
    
    # Create test executable
    add_executable(cgh_tests
        tests/test_game_handler.cpp
        tests/test_integration.cpp
        tests/test_performance.cpp
    )
    
    # Link with libraries
    target_link_libraries(cgh_tests
        PRIVATE cgh
        PRIVATE GTest::gtest
        PRIVATE GTest::gtest_main
    )
    
    # Add test to CTest
    add_test(NAME CGH_Tests COMMAND cgh_tests)
endif()
