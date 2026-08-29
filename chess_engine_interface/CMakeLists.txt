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


