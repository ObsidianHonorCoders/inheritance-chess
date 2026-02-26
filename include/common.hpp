/// @file         common.hpp
/// @brief        Common types and enumerations for the Chess Engine.
/// @project      Inheritance Chess
/// @organization Obsidian Honor Coders
/// @repository   https://github.com/ObsidianHonorCoders/inheritance-chess
/// @author       Calileus
/// @details      Defines shared enumerations for piece colors, piece types, and position
///               structures used throughout the inheritance-chess engine.
/// @date         2026-01-27
/// @copyright    2026 Obsidian Honor Coders
/// @license      Apache License 2.0 - http://www.apache.org/licenses/LICENSE-2.0
/// @note         Licensed under the Apache License, Version 2.0 (the "License");
///               you may not use this file except in compliance with the License.

#ifndef ICHESS_SRC_COMMON
#define ICHESS_SRC_COMMON

/// @struct  Position
/// @brief   Represents a position on the chess board.
/// @details Uses algebraic notation where file (column) ranges from 'a' to 'h'
///          and rank (row) ranges from '1' to '8'.
struct Position
{
    char file = ' '; ///< File (column) coordinate, 'a' through 'h'
    char rank = ' '; ///< Rank (row) coordinate, '1' through '8'
};

/// @brief Default position constant.
/// @note  All fields are initialized to their default values.
/// @see   Position
constexpr Position default_position = {};

/// @struct  Properties
/// @brief   Stores additional information about the board state.
/// @details This struct stores information about the state of the board that is not directly
///          represented by the pieces on the board.
struct Properties
{
    bool     white_king_is_in_check          = false;      ///< Whether the white king is in check.
    bool     black_king_is_in_check          = false;      ///< Whether the black king is in check.
    bool     white_king_has_moved            = false;      ///< Whether the white king has moved.
    bool     black_king_has_moved            = false;      ///< Whether the black king has moved.
    bool     white_rook_king_side_has_moved  = false;      ///< Whether the white rook on the king side has moved.
    bool     white_rook_queen_side_has_moved = false;      ///< Whether the white rook on the queen side has moved.
    bool     black_rook_king_side_has_moved  = false;      ///< Whether the black rook on the king side has moved.
    bool     black_rook_queen_side_has_moved = false;      ///< Whether the black rook on the queen side has moved.
    Position last_move_start                 = {' ', ' '}; ///< The start position of the last move.
    Position last_move_end                   = {' ', ' '}; ///< The end position of the last move.
};

/// @brief Default properties constant.
/// @note  All fields are initialized to their default values.
/// @see   Properties
constexpr Properties default_properties = {};

#endif // ICHESS_SRC_COMMON
