/// @file      common.hpp
/// @brief     Common types and enumerations for the Chess Engine.
/// @author    Calileus
/// @date      2026-01-27
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines shared enumerations for piece colors, piece types, and position
///            structures used throughout the inheritance-chess engine.

#ifndef ICHESS_SRC_COMMON
#define ICHESS_SRC_COMMON

/// @enum    PieceColor
/// @brief   Represents the color of a chess piece.
/// @details Each chess piece belongs to either the white or black side, or no side (NONE).
enum class PieceColor : char
{
  NONE  = ' ', ///< No color assigned
  WHITE = 'w', ///< White piece
  BLACK = 'b'  ///< Black piece
};

/// @enum    PieceType
/// @brief   Represents the type of chess piece.
/// @details Enumerates all standard chess piece types, each with a unique character representation.
enum class PieceType : char
{
  NONE   = ' ', ///< No piece type
  PAWN   = 'P', ///< Pawn piece
  KNIGHT = 'N', ///< Knight piece
  BISHOP = 'B', ///< Bishop piece
  ROOK   = 'R', ///< Rook piece
  QUEEN  = 'Q', ///< Queen piece
  KING   = 'K'  ///< King piece
};

/// @struct  PiecePosition
/// @brief   Represents a position on the chess board.
/// @details Uses algebraic notation where file (column) ranges from 'a' to 'h'
///          and rank (row) ranges from '1' to '8'.
struct PiecePosition
{
    char file; ///< File (column) coordinate, 'a' through 'h'
    char rank; ///< Rank (row) coordinate, '1' through '8'
};

#endif // ICHESS_SRC_COMMON
