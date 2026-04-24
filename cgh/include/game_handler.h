/// @file      game_handler.h
/// @namespace Chess
/// @brief     Chess Game Handler (CGH) - The Orchestrator.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Manages the lifecycle of a chess game session and coordinates
///            all modules. Entry point for the application with unified interface
///            to game logic.

#ifndef ICHESS_CGH_GAME_HANDLER
#define ICHESS_CGH_GAME_HANDLER

#include "grid.h"
#include "move.h"
#include "board_manager.h"
#include "pieces_logic.h"
#include "translation_unit.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace Chess
{

  /// @struct GameSession
  /// @brief  Metadata for a chess game session.
  struct GameSession
  {
      std::string               game_id;                    ///< Unique game identifier
      std::vector<Move>         move_history;               ///< History of all moves played
      std::chrono::milliseconds white_time{600000};         ///< White player time (default 10 min)
      std::chrono::milliseconds black_time{600000};         ///< Black player time (default 10 min)
      GameState                 state = GameState::ONGOING; ///< Current game state
  };

  /// @class   ChessGameHandler
  /// @brief   Orchestrates chess game flow and coordinates all modules.
  /// @details The ChessGameHandler acts as the main entry point, managing game
  ///          lifecycle, coordinating CCI, CPL, CBM, and CTU modules, and
  ///          handling time control. Provides facade pattern interface.
  class ChessGameHandler
  {
    public:
      /// @brief Constructor - initializes all submodules.
      ChessGameHandler();

      /// @brief Start a new game from standard position.
      void start_new_game();

      /// @brief Load game from FEN string.
      void load_game_from_fen(const std::string& fen);

      /// @brief Make a move using algebraic notation.
      bool make_move(const std::string& algebraic_move);

      /// @brief Make a move using Move structure.
      bool make_move(const Move& move);

      /// @brief Get current position as FEN.
      std::string get_current_fen() const;

      /// @brief Get all legal moves in current position.
      std::vector<Move> get_legal_moves() const;

      /// @brief Get all legal moves for a specific grid.
      /// @param grid Grid to analyze.
      /// @return Vector of legal moves.
      std::vector<Move> get_legal_moves(const Grid& grid) const;

      /// @brief Get current game state.
      GameState get_game_state() const;

      /// @brief Start timer for a color.
      void start_timer(Color color);

      /// @brief Stop timer for a color.
      void stop_timer(Color color);

      /// @brief Get time remaining for a color.
      std::chrono::milliseconds get_time_remaining(Color color) const;

      /// @brief Display current board state (adapted from existing Board patterns).
      void display_board() const;

      /// @brief Get current grid state.
      /// @return Reference to current grid.
      const Grid& get_current_grid() const;

    private:
      /// @brief Execute a move on the grid.
      /// @param move Move to execute.
      /// @return True if move was executed successfully.
      bool execute_move_on_grid(const Move& move);

      /// @brief Execute a regular move.
      /// @param move Move to execute.
      /// @return True if move was executed successfully.
      bool execute_regular_move(const Move& move);

      /// @brief Execute castling move.
      /// @param move Castling move to execute.
      /// @return True if castling was executed successfully.
      bool execute_castling(const Move& move);

      /// @brief Execute en passant move.
      /// @param move En passant move to execute.
      /// @return True if en passant was executed successfully.
      bool execute_en_passant(const Move& move);

      /// @brief Execute promotion move.
      /// @param move Promotion move to execute.
      /// @return True if promotion was executed successfully.
      bool execute_promotion(const Move& move);

      /// @brief Update game state based on current position.
      void update_game_state();

      /// @brief Get display character for a piece.
      /// @param type Type of piece.
      /// @param color Color of piece.
      /// @return Character representation.
      char get_piece_display_char(PieceType type, Color color) const;

      std::unique_ptr<ChessBoardManager>    board_manager_;    ///< Board rule enforcement
      std::unique_ptr<ChessPiecesLogic>     pieces_logic_;     ///< Piece move generation
      std::unique_ptr<ChessTranslationUnit> translation_unit_; ///< Notation translation

      Grid        current_grid_; ///< Current board state
      GameSession session_;      ///< Game session metadata
  };

} // namespace Chess

#endif // ICHESS_CGH_GAME_HANDLER
