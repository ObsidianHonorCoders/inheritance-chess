/// @file      board_manager.h
/// @namespace Chess
/// @brief     Chess Board Manager (CBM) - The Referee.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Manages global chess board state and enforces higher-level rules.
///            Verifies move legality, enforces check rule, and detects end-game
///            conditions (checkmate, stalemate, draw).

#ifndef ICHESS_CBM_BOARD_MANAGER
#define ICHESS_CBM_BOARD_MANAGER

#include "grid.h"
#include "move.h"
#include <vector>
#include <memory>

namespace Chess
{

  /// @class   ChessBoardManager
  /// @brief   Manages chess board state and enforces game rules.
  /// @details The ChessBoardManager acts as a referee, enforcing the higher-level
  ///          rules of chess including check detection, move validation, and
  ///          game end condition detection.
  class ChessBoardManager
  {
    public:
      /// @brief Constructor.
      ChessBoardManager() = default;

      /// @brief Check if white king is in check.
      bool is_white_king_in_check(const Grid& grid) const;

      /// @brief Check if black king is in check.
      bool is_black_king_in_check(const Grid& grid) const;

      /// @brief Check if current position is a draw.
      bool is_draw(const Grid& grid, Color color_turn) const;

      /// @brief Validate if a move is legal (doesn't leave king in check).
      bool validate_move(const Grid& grid, const Move& move) const;

      /// @brief Check if position is checkmate.
      bool is_checkmate(const Grid& grid, Color color) const;

      /// @brief Check if position is stalemate.
      bool is_stalemate(const Grid& grid, Color color) const;

      /// @brief Check if a specific king is in check on the board.
      bool is_king_in_check(const Grid& grid, Color king_color) const;

    private:
      Grid current_board_;   ///< Current game board state
      Grid validation_grid_; ///< Validation grid for move testing
  };

} // namespace Chess

#endif // ICHESS_CBM_BOARD_MANAGER
