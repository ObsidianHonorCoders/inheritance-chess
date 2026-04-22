/// @file      chess_pawn.cpp
/// @namespace Chess
/// @brief     Chess Pieces Logic (CPL) - Pawn Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implementation of ChessPawn class with complete pawn movement logic
///            adapted from existing Pawn implementation to work with CCI structures.

#include "chess_pawn.h"
#include <algorithm>
#include <stdexcept>

namespace Chess
{

  /// @brief Construct a ChessPawn at the specified position and color.
  /// @param color The color of the pawn (WHITE or BLACK).
  /// @param pos The initial position of the pawn.
  ChessPawn::ChessPawn(Color color, const Position& pos)
      : color_(color), position_(pos)
  {
      if (!pos.is_valid())
      {
          throw std::invalid_argument("Invalid position for ChessPawn");
      }
  }

  /// @brief Get the character representation of the piece.
  /// @return 'P' for white pawns, 'p' for black pawns.
  char ChessPawn::get_representation() const
  {
      return (color_ == Color::WHITE) ? 'P' : 'p';
  }

  /// @brief Calculate valid moves for this pawn.
  /// @param[out] moves Vector to be filled with valid move positions.
  /// @param[in] other_positions Vector of positions of all other pieces on the board.
  /// @param[in] other_colors Vector of colors corresponding to each piece in other_positions.
  /// @param[in] grid Current grid state for move validation.
  /// @details Adapted from existing Pawn::available_moves implementation.
  void ChessPawn::available_moves(PositionList& moves,
                                 const PositionList& other_positions,
                                 const ColorList& other_colors,
                                 const Grid& grid) const
  {
      moves.clear();
      
      if (other_positions.size() != other_colors.size())
      {
          throw std::invalid_argument("Position and color vectors must have same size");
      }

      int direction = get_direction();
      Position move_candidate;

      // Try to move one square forward if the place is empty
      move_candidate = Position(position_.file, position_.rank + direction);
      if (move_candidate.is_valid())
      {
          auto it = std::find(other_positions.begin(), other_positions.end(), move_candidate);
          if (it == other_positions.end())
          {
              moves.push_back(move_candidate);

              // Try to move two squares forward if in starting position and place is empty
              if (is_in_starting_position())
              {
                  move_candidate = Position(position_.file, position_.rank + 2 * direction);
                  if (move_candidate.is_valid())
                  {
                      auto it2 = std::find(other_positions.begin(), other_positions.end(), move_candidate);
                      if (it2 == other_positions.end())
                      {
                          moves.push_back(move_candidate);
                      }
                  }
              }
          }
      }

      // Capture diagonally left
      move_candidate = Position(position_.file - 1, position_.rank + direction);
      if (move_candidate.is_valid() && can_capture(other_positions, other_colors, move_candidate))
      {
          moves.push_back(move_candidate);
      }

      // Capture diagonally right
      move_candidate = Position(position_.file + 1, position_.rank + direction);
      if (move_candidate.is_valid() && can_capture(other_positions, other_colors, move_candidate))
      {
          moves.push_back(move_candidate);
      }

      // En passant capture left
      move_candidate = Position(position_.file - 1, position_.rank + direction);
      if (move_candidate.is_valid() && can_capture_en_passant(grid, move_candidate))
      {
          moves.push_back(move_candidate);
      }

      // En passant capture right
      move_candidate = Position(position_.file + 1, position_.rank + direction);
      if (move_candidate.is_valid() && can_capture_en_passant(grid, move_candidate))
      {
          moves.push_back(move_candidate);
      }
  }

  /// @brief Get the movement direction for a pawn based on its color.
  /// @return 1 for white pawns (move up), -1 for black pawns (move down).
  /// @details Adapted from existing get_direction function.
  int ChessPawn::get_direction() const
  {
      switch (color_)
      {
      case Color::WHITE:
          return 1;  // White pawns move up (increase rank)
      case Color::BLACK:
          return -1; // Black pawns move down (decrease rank)
      default:
          throw std::runtime_error("Invalid pawn color");
      }
  }

  /// @brief Check if a pawn can capture at the target position.
  /// @param other_positions Vector of positions of all other pieces on the board.
  /// @param other_colors Vector of colors corresponding to each piece in other_positions.
  /// @param target_pos The target position to check for capture.
  /// @return True if the target position contains an opponent piece that can be captured.
  /// @details Adapted from existing try_to_capture function.
  bool ChessPawn::can_capture(const PositionList& other_positions,
                              const ColorList& other_colors,
                              const Position& target_pos) const
  {
      auto it = std::find(other_positions.begin(), other_positions.end(), target_pos);
      if (it != other_positions.end())
      {
          size_t index = std::distance(other_positions.begin(), it);
          Color piece_color = other_colors[index];
          return (piece_color != color_ && piece_color != Color::NONE);
      }
      return false;
  }

  /// @brief Check if en passant capture is possible at the target position.
  /// @param grid Current grid state for move validation.
  /// @param target_pos The target position where en passant capture would occur.
  /// @return True if en passant capture is valid at the target position.
  /// @details Adapted from existing try_to_capture_passant function.
  bool ChessPawn::can_capture_en_passant(const Grid& grid, const Position& target_pos) const
  {
      // En passant is only possible if:
      // 1. The pawn to be captured is on the same rank
      // 2. The opponent pawn just moved two squares forward (halfmove_clock == 0)
      // 3. The last move was a two-square pawn move
      
      if (grid.flags.halfmove_clock != 0)
      {
          return false;
      }

      // The pawn to capture would be one rank behind the target position
      Position piece_to_capture = Position(target_pos.file, target_pos.rank - get_direction());
      
      // Check if there's actually a pawn there that could be captured
      const auto& piece_props = grid.get_piece(piece_to_capture);
      if (!piece_props.has_value())
      {
          return false;
      }

      // Must be an opponent pawn
      if (piece_props->type != PieceType::PAWN || piece_props->color == color_)
      {
          return false;
      }

      // The pawn must have just moved two squares (we can infer this from position)
      // For white pawns, the captured pawn should be on rank 4
      // For black pawns, the captured pawn should be on rank 3
      int expected_rank = (color_ == Color::WHITE) ? 4 : 3;
      return (piece_to_capture.rank == expected_rank);
  }

  /// @brief Check if the pawn is in its starting position.
  /// @return True if pawn is on starting rank (rank 1 for white, rank 6 for black).
  bool ChessPawn::is_in_starting_position() const
  {
      if (color_ == Color::WHITE)
      {
          return position_.rank == 1; // White pawns start on rank 2 (index 1)
      }
      else if (color_ == Color::BLACK)
      {
          return position_.rank == 6; // Black pawns start on rank 7 (index 6)
      }
      return false;
  }

} // namespace Chess
