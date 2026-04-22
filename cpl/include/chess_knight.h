/// @file      chess_knight.h
/// @namespace Chess
/// @brief     Chess Pieces Logic (CPL) - Knight Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines the ChessKnight class that implements knight-specific movement
///            rules in the CPL module, adapted from existing Knight implementation.

#ifndef ICHESS_CPL_CHESS_KNIGHT
#define ICHESS_CPL_CHESS_KNIGHT

#include "../cci/include/grid.h"
#include <vector>

namespace Chess
{

  /// @class ChessKnight
  /// @brief Represents a Knight chess piece in the CPL module.
  /// @details Inherits from ChessPiece and implements knight-specific movement rules.
  ///          Knights move in an L-shape: two squares in one direction and one square perpendicular.
  class ChessKnight : public ChessPiece
  {
  public:
      /// @brief Construct a ChessKnight at the specified position and color.
      /// @param color The color of the knight (WHITE or BLACK).
      /// @param pos The initial position of the knight.
      ChessKnight(Color color, const Position& pos);

      /// @brief Virtual destructor for proper cleanup.
      virtual ~ChessKnight() override = default;

      /// @brief Get the type of the piece.
      /// @return Always returns PieceType::KNIGHT.
      PieceType get_type() const override { return PieceType::KNIGHT; }
      
      /// @brief Get the color of the piece.
      /// @return The color of the knight.
      Color get_color() const override { return color_; }
      
      /// @brief Get the current position of the piece.
      /// @return The current position of the knight.
      Position get_position() const override { return position_; }
      
      /// @brief Set the position of the piece.
      /// @param pos The new position for the knight.
      void set_position(const Position& pos) override { position_ = pos; }
      
      /// @brief Get the character representation of the piece.
      /// @return 'N' for white knights, 'n' for black knights.
      char get_representation() const override;

      /// @brief Calculate valid moves for this knight.
      /// @param[out] moves Vector to be filled with valid move positions.
      /// @param[in] other_positions Vector of positions of all other pieces on the board.
      /// @param[in] other_colors Vector of colors corresponding to each piece in other_positions.
      /// @param[in] grid Current grid state for move validation.
      /// @details Knights move in an L-shape pattern and can jump over other pieces.
      virtual void available_moves(PositionList& moves,
                                 const PositionList& other_positions,
                                 const ColorList& other_colors,
                                 const Grid& grid) const override;

  private:
      Color color_;    ///< Color of the knight (WHITE or BLACK)
      Position position_; ///< Current position of the knight

      /// @brief Knight move offsets (L-shaped moves).
      /// @details Array of 8 possible knight moves: (file_offset, rank_offset)
      static const int knight_moves_[8][2];

      /// @brief Check if a knight can move to the target position.
      /// @param target_pos The target position to check.
      /// @param other_positions Vector of positions of all other pieces on the board.
      /// @param other_colors Vector of colors corresponding to each piece in other_positions.
      /// @return True if the knight can move to the target position.
      bool can_move_to(const Position& target_pos,
                       const PositionList& other_positions,
                       const ColorList& other_colors) const;
  };

} // namespace Chess

#endif // ICHESS_CPL_CHESS_KNIGHT
