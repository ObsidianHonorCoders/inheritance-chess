# Chess Pieces Logic (CPL) - Individual Piece Movement Rules

cmake_minimum_required(VERSION 3.20)

# Create CPL library
add_library(cpl STATIC)

# Source files
target_sources(cpl PRIVATE
    src/chess_bishop.cpp
    src/chess_king.cpp
    src/chess_knight.cpp
    src/chess_pawn.cpp
    src/chess_queen.cpp
    src/chess_rook.cpp
    src/pieces_logic.cpp
    src/piece_factory.cpp
)

# Include directories
target_include_directories(cpl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Dependencies
target_link_libraries(cpl PUBLIC cci)

# Compiler-specific options
if(MSVC)
    target_compile_options(cpl PRIVATE /W4)
else()
    target_compile_options(cpl PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set C++ standard
set_property(TARGET cpl PROPERTY CXX_STANDARD 20)
set_property(TARGET cpl PROPERTY CXX_STANDARD_REQUIRED ON)

# Export symbols for Windows
if(WIN32)
    target_compile_definitions(cpl PRIVATE ICHESS_CPL_EXPORTS)
endif()


