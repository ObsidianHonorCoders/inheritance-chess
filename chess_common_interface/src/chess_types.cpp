/// @file      chess_types.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Common Interface type definitions.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements helper functions for chess type conversions and
///            utilities.

#include "chess_types.h"
#include <string>
#include <stdexcept>

// Implementations for Chess::Color enum helpers

namespace Chess
{

  /// @brief Convert Color to string representation.
  const char* color_to_string(Color color) { return (color == Color::WHITE) ? "White" : "Black"; }

  /// @brief Get opposite color.
  Color get_opposite_color(Color color) { return (color == Color::WHITE) ? Color::BLACK : Color::WHITE; }

  /// @brief Convert PieceType to character representation.
  char piece_type_to_char(PieceType type, Color color)
  {
    char base;
    switch (type)
    {
    case PieceType::PAWN:
      base = 'P';
      break;
    case PieceType::KNIGHT:
      base = 'N';
      break;
    case PieceType::BISHOP:
      base = 'B';
      break;
    case PieceType::ROOK:
      base = 'R';
      break;
    case PieceType::QUEEN:
      base = 'Q';
      break;
    case PieceType::KING:
      base = 'K';
      break;
    default:
      base = '?';
      break;
    }

    // Uppercase for white, lowercase for black
    return (color == Color::WHITE) ? base : (char)(base + 32);
  }

  /// @brief Convert string to PieceType.
  PieceType string_to_piece_type(const std::string& str)
  {
    if (str == "pawn" || str == "P")
      return PieceType::PAWN;
    if (str == "knight" || str == "N")
      return PieceType::KNIGHT;
    if (str == "bishop" || str == "B")
      return PieceType::BISHOP;
    if (str == "rook" || str == "R")
      return PieceType::ROOK;
    if (str == "queen" || str == "Q")
      return PieceType::QUEEN;
    if (str == "king" || str == "K")
      return PieceType::KING;
    throw std::invalid_argument("Invalid piece type: " + str);
  }

} // namespace Chess
