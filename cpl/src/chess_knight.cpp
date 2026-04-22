/// @file      chess_knight.cpp
/// @namespace Chess
/// @brief     Chess Pieces Logic (CPL) - Knight Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implementation of ChessKnight class with complete knight movement logic.

#include "chess_knight.h"
#include <algorithm>
#include <stdexcept>

namespace Chess
{

  /// @brief Knight move offsets (L-shaped moves).
  /// @details Array of 8 possible knight moves: (file_offset, rank_offset)
  const int ChessKnight::knight_moves_[8][2] = {
      { 2,  1},  // Right 2, Up 1
      { 2, -1},  // Right 2, Down 1
      {-2,  1},  // Left 2, Up 1
      {-2, -1},  // Left 2, Down 1
      { 1,  2},  // Right 1, Up 2
      { 1, -2},  // Right 1, Down 2
      {-1,  2},  // Left 1, Up 2
      {-1, -2}   // Left 1, Down 2
  };

  /// @brief Construct a ChessKnight at the specified position and color.
  /// @param color The color of the knight (WHITE or BLACK).
  /// @param pos The initial position of the knight.
  ChessKnight::ChessKnight(Color color, const Position& pos)
      : color_(color), position_(pos)
  {
      if (!pos.is_valid())
      {
          throw std::invalid_argument("Invalid position for ChessKnight");
      }
  }

  /// @brief Get the character representation of the piece.
  /// @return 'N' for white knights, 'n' for black knights.
  char ChessKnight::get_representation() const
  {
      return (color_ == Color::WHITE) ? 'N' : 'n';
  }

  /// @brief Calculate valid moves for this knight.
  /// @param[out] moves Vector to be filled with valid move positions.
  /// @param[in] other_positions Vector of positions of all other pieces on the board.
  /// @param[in] other_colors Vector of colors corresponding to each piece in other_positions.
  /// @param[in] grid Current grid state for move validation.
  /// @details Knights move in an L-shape pattern and can jump over other pieces.
  void ChessKnight::available_moves(PositionList& moves,
                                 const PositionList& other_positions,
                                 const ColorList& other_colors,
                                 const Grid& grid) const
  {
      moves.clear();
      
      if (other_positions.size() != other_colors.size())
      {
          throw std::invalid_argument("Position and color vectors must have same size");
      }

      // Check all 8 possible knight moves
      for (int i = 0; i < 8; i++)
      {
          Position target_pos = Position(
              position_.file + knight_moves_[i][0],
              position_.rank + knight_moves_[i][1]
          );

          // Check if target position is valid and reachable
          if (target_pos.is_valid() && can_move_to(target_pos, other_positions, other_colors))
          {
              moves.push_back(target_pos);
          }
      }
  }

  /// @brief Check if a knight can move to the target position.
  /// @param target_pos The target position to check.
  /// @param other_positions Vector of positions of all other pieces on the board.
  /// @param other_colors Vector of colors corresponding to each piece in other_positions.
  /// @return True if the knight can move to the target position.
  bool ChessKnight::can_move_to(const Position& target_pos,
                               const PositionList& other_positions,
                               const ColorList& other_colors) const
  {
      auto it = std::find(other_positions.begin(), other_positions.end(), target_pos);
      
      // If square is empty, knight can move there
      if (it == other_positions.end())
      {
          return true;
      }
      
      // If square is occupied, knight can only capture opponent pieces
      size_t index = std::distance(other_positions.begin(), it);
      Color piece_color = other_colors[index];
      
      return (piece_color != color_ && piece_color != Color::NONE);
  }

} // namespace Chess
