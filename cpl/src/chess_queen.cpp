/// @file      chess_queen.cpp
/// @namespace Chess
/// @brief     Chess Pieces Logic (CPL) - Queen Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implementation of ChessQueen class with complete queen movement logic.

#include "chess_queen.h"
#include <algorithm>
#include <stdexcept>

namespace Chess
{

  /// @brief Queen move directions (8 directions: 4 orthogonal + 4 diagonal).
  /// @details Array of 8 directions: (file_offset, rank_offset)
  const int ChessQueen::queen_directions_[8][2] = {
      { 1,  0},  // Right (horizontal)
      {-1,  0},  // Left (horizontal)
      { 0,  1},  // Up (vertical)
      { 0, -1},  // Down (vertical)
      { 1,  1},  // Up-right diagonal
      { 1, -1},  // Down-right diagonal
      {-1,  1},  // Up-left diagonal
      {-1, -1}   // Down-left diagonal
  };

  /// @brief Construct a ChessQueen at the specified position and color.
  /// @param color The color of the queen (WHITE or BLACK).
  /// @param pos The initial position of the queen.
  ChessQueen::ChessQueen(Color color, const Position& pos)
      : color_(color), position_(pos)
  {
      if (!pos.is_valid())
      {
          throw std::invalid_argument("Invalid position for ChessQueen");
      }
  }

  /// @brief Get the character representation of the piece.
  /// @return 'Q' for white queens, 'q' for black queens.
  char ChessQueen::get_representation() const
  {
      return (color_ == Color::WHITE) ? 'Q' : 'q';
  }

  /// @brief Calculate valid moves for this queen.
  /// @param[out] moves Vector to be filled with valid move positions.
  /// @param[in] other_positions Vector of positions of all other pieces on the board.
  /// @param[in] other_colors Vector of colors corresponding to each piece in other_positions.
  /// @param[in] grid Current grid state for move validation.
  /// @details Queens move in all 8 directions but are blocked by pieces.
  void ChessQueen::available_moves(PositionList& moves,
                                 const PositionList& other_positions,
                                 const ColorList& other_colors,
                                 const Grid& grid) const
  {
      moves.clear();
      
      if (other_positions.size() != other_colors.size())
      {
          throw std::invalid_argument("Position and color vectors must have same size");
      }

      // Check all 8 directions (4 orthogonal + 4 diagonal)
      for (int i = 0; i < 8; i++)
      {
          int file_offset = queen_directions_[i][0];
          int rank_offset = queen_directions_[i][1];
          
          // Move step by step in the direction
          for (int distance = 1; distance < 8; distance++)
          {
              Position target_pos = Position(
                  position_.file + file_offset * distance,
                  position_.rank + rank_offset * distance
              );

              // Check if target position is still on the board
              if (!target_pos.is_valid())
              {
                  break; // Can't move further in this direction
              }

              // Check if path is clear
              if (!is_path_clear(target_pos, other_positions))
              {
                  // Path is blocked, check if we can capture the blocking piece
                  if (can_move_to(target_pos, other_positions, other_colors))
                  {
                      moves.push_back(target_pos); // Can capture
                  }
                  break; // Can't move further in this direction
              }

              // Path is clear, add this position as a valid move
              moves.push_back(target_pos);
          }
      }
  }

  /// @brief Check if a queen can move to the target position.
  /// @param target_pos The target position to check.
  /// @param other_positions Vector of positions of all other pieces on the board.
  /// @param other_colors Vector of colors corresponding to each piece in other_positions.
  /// @return True if the queen can move to the target position.
  bool ChessQueen::can_move_to(const Position& target_pos,
                             const PositionList& other_positions,
                             const ColorList& other_colors) const
  {
      auto it = std::find(other_positions.begin(), other_positions.end(), target_pos);
      
      // If square is empty, queen can move there
      if (it == other_positions.end())
      {
          return true;
      }
      
      // If square is occupied, queen can only capture opponent pieces
      size_t index = std::distance(other_positions.begin(), it);
      Color piece_color = other_colors[index];
      
      return (piece_color != color_ && piece_color != Color::NONE);
  }

  /// @brief Check if the path to target position is clear.
  /// @param target_pos The target position to reach.
  /// @param other_positions Vector of positions of all other pieces on the board.
  /// @return True if the path is clear, false if blocked.
  bool ChessQueen::is_path_clear(const Position& target_pos,
                               const PositionList& other_positions) const
  {
      // Calculate file and rank differences
      int file_diff = target_pos.file - position_.file;
      int rank_diff = target_pos.rank - position_.rank;
      
      // Determine direction
      int file_dir = (file_diff > 0) ? 1 : (file_diff < 0) ? -1 : 0;
      int rank_dir = (rank_diff > 0) ? 1 : (rank_diff < 0) ? -1 : 0;
      
      // Calculate the maximum distance to check
      int max_distance = std::max(abs(file_diff), abs(rank_diff));

      // Check each square along the path (excluding start and end positions)
      for (int distance = 1; distance < max_distance; distance++)
      {
          Position check_pos = Position(
              position_.file + file_dir * distance,
              position_.rank + rank_dir * distance
          );

          // Check if there's a piece on this square
          auto it = std::find(other_positions.begin(), other_positions.end(), check_pos);
          if (it != other_positions.end())
          {
              return false; // Path is blocked
          }
      }

      return true; // Path is clear
  }

} // namespace Chess
