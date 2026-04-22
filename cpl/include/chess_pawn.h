/// @file      chess_pawn.h
/// @namespace Chess
/// @brief     Chess Pieces Logic (CPL) - Pawn Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines the ChessPawn class that implements pawn-specific movement
///            rules in the CPL module, adapted from existing Pawn implementation.

#ifndef ICHESS_CPL_CHESS_PAWN
#define ICHESS_CPL_CHESS_PAWN

#include "../cci/include/grid.h"
#include <vector>

namespace Chess
{

  /// @class ChessPawn
  /// @brief Represents a Pawn chess piece in the CPL module.
  /// @details Inherits from ChessPiece and implements pawn-specific movement rules.
  ///          Adapted from existing Pawn class to work with CCI namespace and Grid structure.
  class ChessPawn : public ChessPiece
  {
  public:
      /// @brief Construct a ChessPawn at the specified position and color.
      /// @param color The color of the pawn (WHITE or BLACK).
      /// @param pos The initial position of the pawn.
      ChessPawn(Color color, const Position& pos);

      /// @brief Virtual destructor for proper cleanup.
      virtual ~ChessPawn() override = default;

      /// @brief Get the type of the piece.
      /// @return Always returns PieceType::PAWN.
      PieceType get_type() const override { return PieceType::PAWN; }
      
      /// @brief Get the color of the piece.
      /// @return The color of the pawn.
      Color get_color() const override { return color_; }
      
      /// @brief Get the current position of the piece.
      /// @return The current position of the pawn.
      Position get_position() const override { return position_; }
      
      /// @brief Set the position of the piece.
      /// @param pos The new position for the pawn.
      void set_position(const Position& pos) override { position_ = pos; }
      
      /// @brief Get the character representation of the piece.
      /// @return 'P' for white pawns, 'p' for black pawns.
      char get_representation() const override;

      /// @brief Calculate valid moves for this pawn.
      /// @param[out] moves Vector to be filled with valid move positions.
      /// @param[in] other_positions Vector of positions of all other pieces on the board.
      /// @param[in] other_colors Vector of colors corresponding to each piece in other_positions.
      /// @param[in] grid Current grid state for move validation.
      /// @details Adapted from existing Pawn::available_moves implementation.
      virtual void available_moves(PositionList& moves,
                                 const PositionList& other_positions,
                                 const ColorList& other_colors,
                                 const Grid& grid) const override;

  private:
      Color color_;    ///< Color of the pawn (WHITE or BLACK)
      Position position_; ///< Current position of the pawn

      /// @brief Get the movement direction for a pawn based on its color.
      /// @return 1 for white pawns (move up), -1 for black pawns (move down).
      /// @details Adapted from existing get_direction function.
      int get_direction() const;

      /// @brief Check if a pawn can capture at the target position.
      /// @param other_positions Vector of positions of all other pieces on the board.
      /// @param other_colors Vector of colors corresponding to each piece in other_positions.
      /// @param target_pos The target position to check for capture.
      /// @return True if the target position contains an opponent piece that can be captured.
      /// @details Adapted from existing try_to_capture function.
      bool can_capture(const PositionList& other_positions,
                      const ColorList& other_colors,
                      const Position& target_pos) const;

      /// @brief Check if en passant capture is possible at the target position.
      /// @param grid Current grid state for move validation.
      /// @param target_pos The target position where en passant capture would occur.
      /// @return True if en passant capture is valid at the target position.
      /// @details Adapted from existing try_to_capture_passant function.
      bool can_capture_en_passant(const Grid& grid, const Position& target_pos) const;

      /// @brief Check if the pawn is in its starting position.
      /// @return True if pawn is on starting rank (rank 1 for white, rank 6 for black).
      bool is_in_starting_position() const;
  };

} // namespace Chess

#endif // ICHESS_CPL_CHESS_PAWN
