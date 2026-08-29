# Chess Board Manager (CBM) - Game Rules and Validation

cmake_minimum_required(VERSION 3.20)

# Create CBM library
add_library(cbm STATIC)

# Source files
target_sources(cbm PRIVATE
    src/board_manager.cpp
)

# Include directories
target_include_directories(cbm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Dependencies
target_link_libraries(cbm PUBLIC cci cpl)

# Compiler-specific options
if(MSVC)
    target_compile_options(cbm PRIVATE /W4)
else()
    target_compile_options(cbm PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set C++ standard
set_property(TARGET cbm PROPERTY CXX_STANDARD 20)
set_property(TARGET cbm PROPERTY CXX_STANDARD_REQUIRED ON)

# Export symbols for Windows
if(WIN32)
    target_compile_definitions(cbm PRIVATE ICHESS_CBM_EXPORTS)
endif()
