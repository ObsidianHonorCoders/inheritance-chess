/// @file      chess_bishop.h
/// @namespace Chess
/// @brief     Chess Pieces Logic (CPL) - Bishop Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines the ChessBishop class that implements bishop-specific movement
///            rules in the CPL module, adapted from existing Bishop implementation.

#ifndef ICHESS_CPL_CHESS_BISHOP
#define ICHESS_CPL_CHESS_BISHOP

#include "../cci/include/grid.h"
#include <vector>

namespace Chess
{

  /// @class ChessBishop
  /// @brief Represents a Bishop chess piece in the CPL module.
  /// @details Inherits from ChessPiece and implements bishop-specific movement rules.
  ///          Bishops move diagonally any number of squares.
  class ChessBishop : public ChessPiece
  {
  public:
      /// @brief Construct a ChessBishop at the specified position and color.
      /// @param color The color of the bishop (WHITE or BLACK).
      /// @param pos The initial position of the bishop.
      ChessBishop(Color color, const Position& pos);

      /// @brief Virtual destructor for proper cleanup.
      virtual ~ChessBishop() override = default;

      /// @brief Get the type of the piece.
      /// @return Always returns PieceType::BISHOP.
      PieceType get_type() const override { return PieceType::BISHOP; }
      
      /// @brief Get the color of the piece.
      /// @return The color of the bishop.
      Color get_color() const override { return color_; }
      
      /// @brief Get the current position of the piece.
      /// @return The current position of the bishop.
      Position get_position() const override { return position_; }
      
      /// @brief Set the position of the piece.
      /// @param pos The new position for the bishop.
      void set_position(const Position& pos) override { position_ = pos; }
      
      /// @brief Get the character representation of the piece.
      /// @return 'B' for white bishops, 'b' for black bishops.
      char get_representation() const override;

      /// @brief Calculate valid moves for this bishop.
      /// @param[out] moves Vector to be filled with valid move positions.
      /// @param[in] other_positions Vector of positions of all other pieces on the board.
      /// @param[in] other_colors Vector of colors corresponding to each piece in other_positions.
      /// @param[in] grid Current grid state for move validation.
      /// @details Bishops move diagonally without restriction but are blocked by pieces.
      virtual void available_moves(PositionList& moves,
                                 const PositionList& other_positions,
                                 const ColorList& other_colors,
                                 const Grid& grid) const override;

  private:
      Color color_;    ///< Color of the bishop (WHITE or BLACK)
      Position position_; ///< Current position of the bishop

      /// @brief Bishop move directions (4 diagonals).
      /// @details Array of 4 diagonal directions: (file_offset, rank_offset)
      static const int bishop_directions_[4][2];

      /// @brief Check if a bishop can move to the target position.
      /// @param target_pos The target position to check.
      /// @param other_positions Vector of positions of all other pieces on the board.
      /// @param other_colors Vector of colors corresponding to each piece in other_positions.
      /// @return True if the bishop can move to the target position.
      bool can_move_to(const Position& target_pos,
                       const PositionList& other_positions,
                       const ColorList& other_colors) const;

      /// @brief Check if the path to target position is clear.
      /// @param target_pos The target position to reach.
      /// @param other_positions Vector of positions of all other pieces on the board.
      /// @return True if the path is clear, false if blocked.
      bool is_path_clear(const Position& target_pos,
                         const PositionList& other_positions) const;
  };

} // namespace Chess

#endif // ICHESS_CPL_CHESS_BISHOP
