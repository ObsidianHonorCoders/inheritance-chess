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


