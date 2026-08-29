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


