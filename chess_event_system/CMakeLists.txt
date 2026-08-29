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


