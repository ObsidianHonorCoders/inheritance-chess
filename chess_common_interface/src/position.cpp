/// @file      position.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Common Interface Position structure.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements position-related utilities and conversions.

#include "position.h"
#include <string>
#include <stdexcept>

namespace Chess
{

  /// @brief Convert Position to algebraic notation (e.g., "e2", "a8").
  /// @param pos Position to convert.
  /// @return String representation in algebraic notation.
  std::string position_to_algebraic(const Position& pos)
  {
    if (!pos.is_valid())
    {
      throw std::invalid_argument("Invalid position for conversion");
    }

    // Convert file (0-7) to letter (a-h)
    char file_char = static_cast<char>('a' + pos.file);
    // Convert rank (0-7) to number (1-8)
    char rank_char = static_cast<char>('1' + pos.rank);

    return std::string{file_char, rank_char};
  }

  /// @brief Convert algebraic notation to Position (e.g., "a1" -> Position(0, 0)).
  Position algebraic_to_position(const std::string& algebraic)
  {
    if (algebraic.length() != 2)
    {
      throw std::invalid_argument("Invalid algebraic notation: " + algebraic);
    }

    int file = algebraic[0] - 'a';
    int rank = algebraic[1] - '1';

    Position pos(file, rank);
    if (!pos.is_valid())
    {
      throw std::invalid_argument("Position out of bounds: " + algebraic);
    }

    return pos;
  }

} // namespace Chess


