/// @file      chess_king.cpp
/// @namespace Chess
/// @brief     Chess Pieces Logic (CPL) - King Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implementation of ChessKing class with complete king movement logic.

#include "chess_king.h"
#include <algorithm>
#include <stdexcept>

namespace Chess
{

  /// @brief King move directions (8 directions: one square each).
  /// @details Array of 8 directions: (file_offset, rank_offset)
  const int ChessKing::king_moves_[8][2] = {
      { 1,  0},  // Right
      {-1,  0},  // Left
      { 0,  1},  // Up
      { 0, -1},  // Down
      { 1,  1},  // Up-right
      { 1, -1},  // Down-right
      {-1,  1},  // Up-left
      {-1, -1}   // Down-left
  };

  /// @brief Construct a ChessKing at the specified position and color.
  /// @param color The color of the king (WHITE or BLACK).
  /// @param pos The initial position of the king.
  ChessKing::ChessKing(Color color, const Position& pos)
      : color_(color), position_(pos)
  {
      if (!pos.is_valid())
      {
          throw std::invalid_argument("Invalid position for ChessKing");
      }
  }

  /// @brief Get the character representation of the piece.
  /// @return 'K' for white kings, 'k' for black kings.
  char ChessKing::get_representation() const
  {
      return (color_ == Color::WHITE) ? 'K' : 'k';
  }

  /// @brief Calculate valid moves for this king.
  /// @param[out] moves Vector to be filled with valid move positions.
  /// @param[in] other_positions Vector of positions of all other pieces on the board.
  /// @param[in] other_colors Vector of colors corresponding to each piece in other_positions.
  /// @param[in] grid Current grid state for move validation.
  /// @details Kings move one square in any direction and can castle.
  void ChessKing::available_moves(PositionList& moves,
                                 const PositionList& other_positions,
                                 const ColorList& other_colors,
                                 const Grid& grid) const
  {
      moves.clear();
      
      if (other_positions.size() != other_colors.size())
      {
          throw std::invalid_argument("Position and color vectors must have same size");
      }

      // Check all 8 possible king moves (one square each)
      for (int i = 0; i < 8; i++)
      {
          Position target_pos = Position(
              position_.file + king_moves_[i][0],
              position_.rank + king_moves_[i][1]
          );

          // Check if target position is valid and reachable
          if (target_pos.is_valid() && can_move_to(target_pos, other_positions, other_colors))
          {
              moves.push_back(target_pos);
          }
      }

      // Check castling moves (if king hasn't moved and path is clear)
      // Kingside castling
      if (can_castle(true, grid, other_positions))
      {
          moves.push_back(Position(position_.file + 2, position_.rank));
      }

      // Queenside castling
      if (can_castle(false, grid, other_positions))
      {
          moves.push_back(Position(position_.file - 2, position_.rank));
      }
  }

  /// @brief Check if a king can move to the target position.
  /// @param target_pos The target position to check.
  /// @param other_positions Vector of positions of all other pieces on the board.
  /// @param other_colors Vector of colors corresponding to each piece in other_positions.
  /// @return True if the king can move to the target position.
  bool ChessKing::can_move_to(const Position& target_pos,
                             const PositionList& other_positions,
                             const ColorList& other_colors) const
  {
      auto it = std::find(other_positions.begin(), other_positions.end(), target_pos);
      
      // If square is empty, king can move there
      if (it == other_positions.end())
      {
          return true;
      }
      
      // If square is occupied, king can only capture opponent pieces
      size_t index = std::distance(other_positions.begin(), it);
      Color piece_color = other_colors[index];
      
      return (piece_color != color_ && piece_color != Color::NONE);
  }

  /// @brief Check if castling is possible in the specified direction.
  /// @param is_kingside True for kingside castling, false for queenside.
  /// @param grid Current grid state for move validation.
  /// @param other_positions Vector of positions of all other pieces on the board.
  /// @return True if castling is possible.
  bool ChessKing::can_castle(bool is_kingside,
                           const Grid& grid,
                           const PositionList& other_positions) const
  {
      // Check if king is in starting position
      Position king_start_pos = (color_ == Color::WHITE) ? Position(4, 0) : Position(4, 7);
      if (position_ != king_start_pos)
      {
          return false;
      }

      // Check if castling rights are available
      if (color_ == Color::WHITE)
      {
          if (is_kingside && !grid.flags.white_can_castle_kingside)
          {
              return false;
          }
          if (!is_kingside && !grid.flags.white_can_castle_queenside)
          {
              return false;
          }
      }
      else
      {
          if (is_kingside && !grid.flags.black_can_castle_kingside)
          {
              return false;
          }
          if (!is_kingside && !grid.flags.black_can_castle_queenside)
          {
              return false;
          }
      }

      // Check if path is clear between king and rook
      int start_file = position_.file;
      int end_file = is_kingside ? 7 : 0;
      int direction = is_kingside ? 1 : -1;

      // Check squares between king and rook
      for (int file = start_file + direction; file != end_file; file += direction)
      {
          Position check_pos = Position(file, position_.rank);
          auto it = std::find(other_positions.begin(), other_positions.end(), check_pos);
          if (it != other_positions.end())
          {
              return false; // Path is blocked
          }
      }

      // Check if rook is in correct position
      Position rook_pos = Position(end_file, position_.rank);
      auto it = std::find(other_positions.begin(), other_positions.end(), rook_pos);
      if (it == other_positions.end())
      {
          return false; // No rook found
      }

      // Check if the piece is actually a rook of the same color
      size_t index = std::distance(other_positions.begin(), it);
      // Note: In a full implementation, we'd need to check piece types
      // For now, we assume it's a rook if it's at the correct position

      return true;
  }

} // namespace Chess
