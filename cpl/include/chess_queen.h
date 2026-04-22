/// @file      chess_queen.h
/// @namespace Chess
/// @brief     Chess Pieces Logic (CPL) - Queen Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines the ChessQueen class that implements queen-specific movement
///            rules in the CPL module, adapted from existing Queen implementation.

#ifndef ICHESS_CPL_CHESS_QUEEN
#define ICHESS_CPL_CHESS_QUEEN

#include "../cci/include/grid.h"
#include <vector>

namespace Chess
{

  /// @class ChessQueen
  /// @brief Represents a Queen chess piece in the CPL module.
  /// @details Inherits from ChessPiece and implements queen-specific movement rules.
  ///          Queens move horizontally, vertically, or diagonally any number of squares.
  class ChessQueen : public ChessPiece
  {
  public:
      /// @brief Construct a ChessQueen at the specified position and color.
      /// @param color The color of the queen (WHITE or BLACK).
      /// @param pos The initial position of the queen.
      ChessQueen(Color color, const Position& pos);

      /// @brief Virtual destructor for proper cleanup.
      virtual ~ChessQueen() override = default;

      /// @brief Get the type of the piece.
      /// @return Always returns PieceType::QUEEN.
      PieceType get_type() const override { return PieceType::QUEEN; }
      
      /// @brief Get the color of the piece.
      /// @return The color of the queen.
      Color get_color() const override { return color_; }
      
      /// @brief Get the current position of the piece.
      /// @return The current position of the queen.
      Position get_position() const override { return position_; }
      
      /// @brief Set the position of the piece.
      /// @param pos The new position for the queen.
      void set_position(const Position& pos) override { position_ = pos; }
      
      /// @brief Get the character representation of the piece.
      /// @return 'Q' for white queens, 'q' for black queens.
      char get_representation() const override;

      /// @brief Calculate valid moves for this queen.
      /// @param[out] moves Vector to be filled with valid move positions.
      /// @param[in] other_positions Vector of positions of all other pieces on the board.
      /// @param[in] other_colors Vector of colors corresponding to each piece in other_positions.
      /// @param[in] grid Current grid state for move validation.
      /// @details Queens move in all 8 directions but are blocked by pieces.
      virtual void available_moves(PositionList& moves,
                                 const PositionList& other_positions,
                                 const ColorList& other_colors,
                                 const Grid& grid) const override;

  private:
      Color color_;    ///< Color of the queen (WHITE or BLACK)
      Position position_; ///< Current position of the queen

      /// @brief Queen move directions (8 directions: 4 orthogonal + 4 diagonal).
      /// @details Array of 8 directions: (file_offset, rank_offset)
      static const int queen_directions_[8][2];

      /// @brief Check if a queen can move to the target position.
      /// @param target_pos The target position to check.
      /// @param other_positions Vector of positions of all other pieces on the board.
      /// @param other_colors Vector of colors corresponding to each piece in other_positions.
      /// @return True if the queen can move to the target position.
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

#endif // ICHESS_CPL_CHESS_QUEEN
