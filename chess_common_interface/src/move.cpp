/// @file      move.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Common Interface Move structure.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements move-related utilities and conversions.

#include "move.h"
#include <string>
#include <stdexcept>

namespace Chess
{

  /// @brief Convert Move to algebraic notation (e.g., "e2e4" or "e1g1" for castling).
  std::string move_to_algebraic(const Move& move)
  {
    return position_to_algebraic(move.start_pos) + position_to_algebraic(move.end_pos);
  }

  /// @brief Convert algebraic notation to Move (e.g., "e2e4" -> Move).
  Move algebraic_to_move(const std::string& algebraic)
  {
    if (algebraic.length() < 4)
    {
      throw std::invalid_argument("Invalid algebraic notation for move: " + algebraic);
    }

    Position start = algebraic_to_position(algebraic.substr(0, 2));
    Position end   = algebraic_to_position(algebraic.substr(2, 2));

    Move move(start, end);

    // Handle promotion if specified
    if (algebraic.length() > 4)
    {
      char promotion_char = algebraic[4];
      switch (promotion_char)
      {
      case 'q':
        move.promotion_piece = PieceType::QUEEN;
        break;
      case 'r':
        move.promotion_piece = PieceType::ROOK;
        break;
      case 'b':
        move.promotion_piece = PieceType::BISHOP;
        break;
      case 'n':
        move.promotion_piece = PieceType::KNIGHT;
        break;
      default:
        throw std::invalid_argument("Invalid promotion piece: " + std::string(1, promotion_char));
      }
      move.flags = SpecialFlags::PROMOTION;
    }

    return move;
  }

} // namespace Chess


