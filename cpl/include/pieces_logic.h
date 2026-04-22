/// @file      pieces_logic.h
/// @namespace Chess
/// @brief     Chess Pieces Logic (CPL) - The Physicist.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Handles how individual chess pieces move in isolation, ignoring
///            the "Check" rule. Calculates raw logical moves and determines
///            square attacks.

#ifndef ICHESS_CPL_PIECES_LOGIC
#define ICHESS_CPL_PIECES_LOGIC

#include "../cci/include/grid.h"
#include "../cci/include/move.h"
#include <vector>
#include <memory>

namespace Chess
{

  /// @class   ChessPiecesLogic
  /// @brief   Calculates piece moves based on piece type and board geometry.
  /// @details The ChessPiecesLogic class calculates "Raw Logical Moves" for each
  ///          piece type and determines if squares are attacked, without
  ///          considering the check rule.
  class ChessPiecesLogic
  {
    public:
      /// @brief Constructor.
      ChessPiecesLogic() = default;

      /// @brief Check if a move is a valid raw logical move.
      bool is_raw_logical_move(const Grid& grid, const Move& move) const;

      /// @brief Get all raw logical moves from a position.
      std::vector<Move> list_raw_logical_moves(const Grid& grid, Position position) const;

      /// @brief Check if a square is attacked by a color.
      bool is_square_attacked(const Grid& grid, Position square, Color by_color) const;

    private:
      /// @brief Get pawn moves for a given position.
      std::vector<Move> get_pawn_moves(const Grid& grid, Position pos) const;
      /// @brief Get knight moves for a given position.
      std::vector<Move> get_knight_moves(const Grid& grid, Position pos) const;
      /// @brief Get bishop moves for a given position.
      std::vector<Move> get_bishop_moves(const Grid& grid, Position pos) const;
      /// @brief Get rook moves for a given position.
      std::vector<Move> get_rook_moves(const Grid& grid, Position pos) const;
      /// @brief Get queen moves for a given position.
      std::vector<Move> get_queen_moves(const Grid& grid, Position pos) const;
      /// @brief Get king moves for a given position.
      std::vector<Move> get_king_moves(const Grid& grid, Position pos) const;

      // Helper methods
      /// @brief Calculate sliding piece moves in given directions.
      std::vector<Move>
      get_sliding_moves(const Grid& grid, Position pos, const std::vector<std::pair<int, int>>& directions) const;
  };

} // namespace Chess

#endif // ICHESS_CPL_PIECES_LOGIC
