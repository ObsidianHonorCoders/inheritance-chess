# Chess Translation Unit (CTU) - Notation Parsing and Conversion

cmake_minimum_required(VERSION 3.20)

# Create CTU library
add_library(ctu STATIC)

# Source files
target_sources(ctu PRIVATE
    src/translation_unit.cpp
)

# Include directories
target_include_directories(ctu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(ctu PRIVATE
    ${CMAKE_SOURCE_DIR}/chess_game_handler/include
    ${CMAKE_SOURCE_DIR}/chess_engine_interface/include
    ${CMAKE_SOURCE_DIR}/chess_pieces_logic/include
    ${CMAKE_SOURCE_DIR}/chess_event_system/include
)

# Dependencies
target_link_libraries(ctu PUBLIC cci cbm)

# Compiler-specific options
if(MSVC)
    target_compile_options(ctu PRIVATE /W4)
else()
    target_compile_options(ctu PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set C++ standard
set_property(TARGET ctu PROPERTY CXX_STANDARD 20)
set_property(TARGET ctu PROPERTY CXX_STANDARD_REQUIRED ON)

# Export symbols for Windows
if(WIN32)
    target_compile_definitions(ctu PRIVATE ICHESS_CTU_EXPORTS)
endif()
