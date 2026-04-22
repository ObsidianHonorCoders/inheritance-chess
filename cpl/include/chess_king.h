/// @file      chess_king.h
/// @namespace Chess
/// @brief     Chess Pieces Logic (CPL) - King Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines the ChessKing class that implements king-specific movement
///            rules in the CPL module, adapted from existing King implementation.

#ifndef ICHESS_CPL_CHESS_KING
#define ICHESS_CPL_CHESS_KING

#include "../cci/include/grid.h"
#include <vector>

namespace Chess
{

  /// @class ChessKing
  /// @brief Represents a King chess piece in the CPL module.
  /// @details Inherits from ChessPiece and implements king-specific movement rules.
  ///          Kings move one square in any direction (including castling).
  class ChessKing : public ChessPiece
  {
  public:
      /// @brief Construct a ChessKing at the specified position and color.
      /// @param color The color of the king (WHITE or BLACK).
      /// @param pos The initial position of the king.
      ChessKing(Color color, const Position& pos);

      /// @brief Virtual destructor for proper cleanup.
      virtual ~ChessKing() override = default;

      /// @brief Get the type of the piece.
      /// @return Always returns PieceType::KING.
      PieceType get_type() const override { return PieceType::KING; }
      
      /// @brief Get the color of the piece.
      /// @return The color of the king.
      Color get_color() const override { return color_; }
      
      /// @brief Get the current position of the piece.
      /// @return The current position of the king.
      Position get_position() const override { return position_; }
      
      /// @brief Set the position of the piece.
      /// @param pos The new position for the king.
      void set_position(const Position& pos) override { position_ = pos; }
      
      /// @brief Get the character representation of the piece.
      /// @return 'K' for white kings, 'k' for black kings.
      char get_representation() const override;

      /// @brief Calculate valid moves for this king.
      /// @param[out] moves Vector to be filled with valid move positions.
      /// @param[in] other_positions Vector of positions of all other pieces on the board.
      /// @param[in] other_colors Vector of colors corresponding to each piece in other_positions.
      /// @param[in] grid Current grid state for move validation.
      /// @details Kings move one square in any direction and can castle.
      virtual void available_moves(PositionList& moves,
                                 const PositionList& other_positions,
                                 const ColorList& other_colors,
                                 const Grid& grid) const override;

  private:
      Color color_;    ///< Color of the king (WHITE or BLACK)
      Position position_; ///< Current position of the king

      /// @brief King move directions (8 directions: one square each).
      /// @details Array of 8 directions: (file_offset, rank_offset)
      static const int king_moves_[8][2];

      /// @brief Check if a king can move to the target position.
      /// @param target_pos The target position to check.
      /// @param other_positions Vector of positions of all other pieces on the board.
      /// @param other_colors Vector of colors corresponding to each piece in other_positions.
      /// @return True if the king can move to the target position.
      bool can_move_to(const Position& target_pos,
                       const PositionList& other_positions,
                       const ColorList& other_colors) const;

      /// @brief Check if castling is possible in the specified direction.
      /// @param is_kingside True for kingside castling, false for queenside.
      /// @param grid Current grid state for move validation.
      /// @param other_positions Vector of positions of all other pieces on the board.
      /// @return True if castling is possible.
      bool can_castle(bool is_kingside,
                     const Grid& grid,
                     const PositionList& other_positions) const;
  };

} // namespace Chess

#endif // ICHESS_CPL_CHESS_KING
