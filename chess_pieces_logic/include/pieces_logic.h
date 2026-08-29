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

#include "grid.h"
#include "move.h"
#include <cstdint>
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

      /// @brief Build a cached occupied-square snapshot for one move-generation pass.
      /// @param grid Grid to scan.
      /// @param positions Output occupied positions.
      /// @param colors Output colors for positions (same index mapping as positions).
      void build_piece_cache(const Grid& grid, PositionList& positions, ColorList& colors) const;

      /// @brief Get raw logical moves from a position using a prebuilt occupancy cache.
      /// @param grid Grid reference.
      /// @param position Origin square.
      /// @param positions Cached occupied positions from build_piece_cache.
      /// @param colors Cached colors corresponding to positions.
      std::vector<Move> list_raw_logical_moves_cached(const Grid&             grid,
                      Position                position,
                      const PositionList&     positions,
                      const ColorList&        colors) const;

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
        /// @brief Reuse cached board occupancy snapshot for repeated per-position queries.
        void get_or_build_cached_board_data(const Grid& grid,
                          const PositionList*& positions,
                          const ColorList*& colors) const;

        mutable bool cache_valid_ = false;
        mutable uint64_t cache_occupied_ = 0;
        mutable uint64_t cache_white_occupied_ = 0;
        mutable uint64_t cache_black_occupied_ = 0;
        mutable PositionList cached_positions_;
        mutable ColorList cached_colors_;
  };

} // namespace Chess

#endif // ICHESS_CPL_PIECES_LOGIC
