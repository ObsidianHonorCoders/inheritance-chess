/// @file      chess_engine.h
/// @namespace Chess
/// @brief     Chess Engine Interface (CEI) - The Brain.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Provides high-level chess engine interface for move evaluation,
///            search algorithms, and AI decision making. Builds on existing
///            move generation framework and position analysis patterns.

#ifndef ICHESS_CEI_CHESS_ENGINE
#define ICHESS_CEI_CHESS_ENGINE

#include "grid.h"
#include "move.h"
#include "game_handler.h"
#include <memory>
#include <vector>
#include <chrono>

namespace Chess
{

  /// @struct  EvaluationResult
  /// @brief   Result of position evaluation with score and best move.
  struct EvaluationResult
  {
      int        score;           ///< Position evaluation score (centipawns)
      Move       best_move;      ///< Best move found
      int        depth;           ///< Search depth reached
      uint64_t   nodes_searched;  ///< Number of nodes examined
      std::chrono::milliseconds time_used; ///< Time spent searching
  };

  /// @struct  SearchLimits
  /// @brief   Limits for search algorithm parameters.
  struct SearchLimits
  {
      int max_depth = 20;                        ///< Maximum search depth
      std::chrono::milliseconds max_time{5000}; ///< Maximum search time
      uint64_t max_nodes = 1000000;               ///< Maximum nodes to search
      bool infinite = false;                     ///< Infinite search mode
  };

  /// @class   ChessEngine
  /// @brief   High-level chess engine interface for AI decision making.
  /// @details The ChessEngine provides move evaluation, search algorithms,
  ///          and AI capabilities building on the existing modular architecture.
  ///          Implements minimax with alpha-beta pruning and position evaluation.
  class ChessEngine
  {
    public:
      /// @brief Constructor - initializes engine components.
      ChessEngine();

      /// @brief Destructor - cleans up engine resources.
      ~ChessEngine() = default;

      /// @brief Find the best move for the current position.
      /// @param grid Current board position.
      /// @param limits Search constraints.
      /// @return Evaluation result with best move and score.
      EvaluationResult find_best_move(const Grid& grid, const SearchLimits& limits = SearchLimits());

      /// @brief Evaluate a chess position statically.
      /// @param grid Position to evaluate.
      /// @param color Color to evaluate from.
      /// @return Position score in centipawns.
      int evaluate_position(const Grid& grid, Color color);

      /// @brief Get all legal moves for current position.
      /// @param grid Current board position.
      /// @return Vector of legal moves.
      std::vector<Move> get_legal_moves(const Grid& grid);

      /// @brief Check if a position is a draw.
      /// @param grid Position to check.
      /// @return True if position is a draw.
      bool is_draw(const Grid& grid);

      /// @brief Set engine difficulty level.
      /// @param difficulty Difficulty from 1-10.
      void set_difficulty(int difficulty);

      /// @brief Get current engine statistics.
      /// @return Statistics about engine performance.
      std::string get_statistics() const;

      /// @brief Reset engine statistics.
      void reset_statistics();

      /// @brief Evaluate material balance (for testing).
      /// @param grid Position to evaluate.
      /// @return Material score in centipawns.
      int evaluate_material(const Grid& grid);

    private:
      std::unique_ptr<ChessGameHandler> game_handler_; ///< Game handler for move validation
      int difficulty_level_;                         ///< Current engine difficulty (1-10)
      
      // Search statistics
      uint64_t total_nodes_searched_;                ///< Total nodes searched in all searches
      std::chrono::milliseconds total_search_time_;  ///< Total time spent searching
      uint64_t searches_performed_;                  ///< Number of searches performed

      /// @brief Minimax search with alpha-beta pruning.
      /// @param grid Current position.
      /// @param depth Current search depth.
      /// @param alpha Alpha value for pruning.
      /// @param beta Beta value for pruning.
      /// @param maximizing True if maximizing player.
      /// @param limits Search constraints.
      /// @return Evaluation score.
      int minimax(const Grid& grid, int depth, int alpha, int beta, bool maximizing, 
                  const SearchLimits& limits);

      /// @brief Quiescence search for tactical positions.
      /// @param grid Current position.
      /// @param alpha Alpha value for pruning.
      /// @param beta Beta value for pruning.
      /// @return Quiescence evaluation score.
      int quiescence_search(const Grid& grid, int alpha, int beta);

      
      /// @brief Evaluate piece positions and mobility.
      /// @param grid Position to evaluate.
      /// @return Positional score in centipawns.
      int evaluate_positional(const Grid& grid);

      /// @brief Evaluate king safety.
      /// @param grid Position to evaluate.
      /// @param color King color to evaluate.
      /// @return King safety score in centipawns.
      int evaluate_king_safety(const Grid& grid, Color color);

      /// @brief Get piece value for evaluation.
      /// @param type Piece type.
      /// @return Piece value in centipawns.
      int get_piece_value(PieceType type) const;

      /// @brief Check if time limit exceeded.
      /// @param start_time Search start time.
      /// @param limits Search constraints.
      /// @return True if time limit exceeded.
      bool is_time_limit_exceeded(const std::chrono::steady_clock::time_point& start_time,
                                  const SearchLimits& limits) const;
  };

} // namespace Chess

#endif // ICHESS_CEI_CHESS_ENGINE
