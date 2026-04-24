# Chess Event System (CES) - Event-Driven Architecture

cmake_minimum_required(VERSION 3.20)

# Create CES library
add_library(ces STATIC)

# Source files
target_sources(ces PRIVATE
    src/event_system.cpp
)

# Include directories
target_include_directories(ces PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Dependencies
target_link_libraries(ces PUBLIC cci)

# Compiler-specific options
if(MSVC)
    target_compile_options(ces PRIVATE /W4)
else()
    target_compile_options(ces PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set C++ standard
set_property(TARGET ces PROPERTY CXX_STANDARD 20)
set_property(TARGET ces PROPERTY CXX_STANDARD_REQUIRED ON)

# Export symbols for Windows
if(WIN32)
    target_compile_definitions(ces PRIVATE ICHESS_CES_EXPORTS)
endif()

# Optional: Build tests if requested
option(BUILD_CES_TESTS "Build CES tests" OFF)
if(BUILD_CES_TESTS)
    # Enable testing
    enable_testing()
    
    # Find Google Test
    find_package(GTest REQUIRED)
    
    # Create test executable
    add_executable(ces_tests)
    
    # Link with libraries
    target_link_libraries(ces_tests
        PRIVATE ces
        PRIVATE GTest::gtest
        PRIVATE GTest::gtest_main
    )
    
    # Add test to CTest
    add_test(NAME CES_Tests COMMAND ces_tests)
endif()
