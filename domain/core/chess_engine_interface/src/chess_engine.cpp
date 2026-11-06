/// @file      chess_engine.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Engine Interface (CEI).
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements move evaluation, search algorithms, and AI capabilities.

#include "chess_engine.h"
#include "grid.h"
#include "board_manager.h"
#include <algorithm>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdint>

namespace Chess
{

  namespace
  {
    uint64_t splitmix64(uint64_t x)
    {
      x += 0x9E3779B97F4A7C15ULL;
      x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
      x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
      return x ^ (x >> 31);
    }
  } // namespace

  ChessEngine::ChessEngine()
      : difficulty_level_(5), total_nodes_searched_(0), total_search_time_(std::chrono::milliseconds(0)),
        searches_performed_(0)
  {
    uint64_t seed = 0xC0FFEE1234567890ULL;

    for (size_t color = 0; color < 2; ++color)
    {
      for (size_t piece = 0; piece < 6; ++piece)
      {
        for (size_t square = 0; square < 64; ++square)
        {
          zobrist_piece_table_[color][piece][square] = splitmix64(seed++);
        }
      }
    }

    zobrist_side_to_move_ = splitmix64(seed++);
    for (size_t i = 0; i < zobrist_castling_rights_.size(); ++i)
    {
      zobrist_castling_rights_[i] = splitmix64(seed++);
    }
  }

  /// @brief Find best move.
  EvaluationResult ChessEngine::find_best_move(const Grid& grid, const SearchLimits& limits)
  {
    EvaluationResult result;
    result.depth          = 0;
    result.nodes_searched = 0;
    result.score          = 0;

    search_start_time_   = std::chrono::steady_clock::now();
    current_limits_      = limits;
    nodes_searched_current_ = 0;
    transposition_table_.clear();

    // Get legal moves for current position
    auto legal_moves = get_legal_moves(grid);
    const uint64_t root_hash = compute_position_hash(grid);

    if (legal_moves.empty())
    {
      // No legal moves - game over
      result.score = (grid.current_turn == Color::WHITE) ? -10000 : 10000;
      return result;
    }

    // Adjust search depth based on difficulty
    int max_depth = std::min(limits.max_depth, difficulty_level_ + 3);

    // Iterative deepening improves move ordering and produces a valid move under tight time caps.
    Move principal_variation_move = legal_moves[0];
    int  principal_score          = -99999;
    int  completed_depth          = 0;

    for (int depth = 1; depth <= max_depth; ++depth)
    {
      if (should_stop_search())
      {
        break;
      }

      std::vector<Move> ordered_moves = legal_moves;
      auto pv_it = std::find(ordered_moves.begin(), ordered_moves.end(), principal_variation_move);
      if (pv_it != ordered_moves.end())
      {
        std::iter_swap(ordered_moves.begin(), pv_it);
      }

      Move iteration_best_move = ordered_moves[0];
      int  iteration_best_score = -99999;

      for (const auto& move : ordered_moves)
      {
        if (should_stop_search())
        {
          break;
        }

        Grid next_grid = grid.clone();
        UndoRecord undo = next_grid.apply_move_inplace(move);
        const uint64_t child_hash = update_zobrist_hash_for_move(root_hash, move, undo, next_grid);

        const int score = minimax(next_grid, depth - 1, -99999, 99999, false, limits, child_hash);

        if (score > iteration_best_score)
        {
          iteration_best_score = score;
          iteration_best_move  = move;
        }
      }

      // Accept this depth only if it completed without a hard stop.
      if (should_stop_search())
      {
        break;
      }

      principal_variation_move = iteration_best_move;
      principal_score          = iteration_best_score;
      completed_depth          = depth;
    }

    result.best_move = principal_variation_move;
    result.score     = principal_score;
    result.depth     = (completed_depth > 0) ? completed_depth : 1;
    result.nodes_searched = nodes_searched_current_;

    auto end_time    = std::chrono::steady_clock::now();
    result.time_used = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - search_start_time_);

    // Update statistics
    total_nodes_searched_ += result.nodes_searched;
    total_search_time_ += result.time_used;
    searches_performed_++;

    return result;
  }

  /// @brief Evaluate position.
  int ChessEngine::evaluate_position(const Grid& grid, Color color)
  {
    int material_score    = evaluate_material(grid);
    int positional_score  = evaluate_positional(grid);
    int king_safety_score = evaluate_king_safety(grid, color)
                            - evaluate_king_safety(grid, (color == Color::WHITE) ? Color::BLACK : Color::WHITE);

    int total_score = material_score + positional_score + king_safety_score;

    // Adjust score based on perspective
    if (color == Color::BLACK)
    {
      total_score = -total_score;
    }

    return total_score;
  }

  /// @brief Get legal moves.
  std::vector<Move> ChessEngine::get_legal_moves(const Grid& grid)
  {
    // Delegate to board manager for consistent legal move generation
    ChessBoardManager board_manager;
    return board_manager.get_legal_moves(grid);
  }

  /// @brief Return whether draw.
  bool ChessEngine::is_draw(const Grid& grid)
  {
    ChessBoardManager board_manager;
    return board_manager.is_draw(grid, grid.current_turn);
  }

  /// @brief Set difficulty.
  void ChessEngine::set_difficulty(int difficulty) { difficulty_level_ = std::clamp(difficulty, 1, 10); }

  std::string ChessEngine::get_statistics() const
  {
    std::string stats = "Chess Engine Statistics:\n";
    stats += "Difficulty Level: " + std::to_string(difficulty_level_) + "\n";
    stats += "Total Searches: " + std::to_string(searches_performed_) + "\n";
    stats += "Total Nodes Searched: " + std::to_string(total_nodes_searched_) + "\n";
    stats += "Total Search Time: " + std::to_string(total_search_time_.count()) + " ms\n";

    if (searches_performed_ > 0)
    {
      uint64_t avg_nodes = total_nodes_searched_ / searches_performed_;
      uint64_t avg_time  = total_search_time_.count() / searches_performed_;
      stats += "Average Nodes per Search: " + std::to_string(avg_nodes) + "\n";
      stats += "Average Time per Search: " + std::to_string(avg_time) + " ms\n";
    }

    return stats;
  }

  /// @brief Reset statistics.
  void ChessEngine::reset_statistics()
  {
    total_nodes_searched_ = 0;
    total_search_time_    = std::chrono::milliseconds(0);
    searches_performed_   = 0;
  }

    /// @brief Run minimax search with alpha-beta pruning.
    int ChessEngine::minimax(
      Grid& grid, int depth, int alpha, int beta, bool maximizing, const SearchLimits& limits, uint64_t hash_key)
  {
    (void)limits;
    nodes_searched_current_++;

    if (should_stop_search())
    {
      return evaluate_position(grid, grid.current_turn);
    }

    const int alpha_original = alpha;
    Move tt_move;
    int tt_score = 0;
    if (probe_transposition(hash_key, depth, alpha, beta, tt_score, &tt_move))
    {
      return tt_score;
    }

    // Base case: leaf node
    if (depth == 0)
    {
      return quiescence_search(grid, alpha, beta, hash_key);
    }

    auto legal_moves = get_legal_moves(grid);

    if (legal_moves.empty())
    {
      // Game over - evaluate terminal position
      Color current_color = grid.current_turn;
      if (maximizing)
      {
        return (current_color == Color::WHITE) ? -10000 : 10000;
      }
      else
      {
        return (current_color == Color::WHITE) ? 10000 : -10000;
      }
    }

    int best_score = maximizing ? -99999 : 99999;
    Move best_move = legal_moves.front();

    // TT move ordering: search remembered principal move first when available.
    if (tt_move.start_pos.is_valid() && tt_move.end_pos.is_valid())
    {
      auto it = std::find(legal_moves.begin(), legal_moves.end(), tt_move);
      if (it != legal_moves.end())
      {
        std::iter_swap(legal_moves.begin(), it);
      }
    }

    for (const auto& move : legal_moves)
    {
      if (should_stop_search())
      {
        break;
      }

      // Apply move in-place to avoid allocation.
      UndoRecord undo = grid.apply_move_inplace(move);
      const uint64_t child_hash = update_zobrist_hash_for_move(hash_key, move, undo, grid);

      int score = minimax(grid, depth - 1, alpha, beta, !maximizing, limits, child_hash);

      // Undo move to restore state.
      grid.undo_move(move, undo);

      if (maximizing)
      {
        if (score > best_score)
        {
          best_score = score;
          best_move = move;
        }
        alpha = std::max(alpha, score);
      }
      else
      {
        if (score < best_score)
        {
          best_score = score;
          best_move = move;
        }
        beta = std::min(beta, score);
      }

      // Alpha-beta pruning
      if (beta <= alpha)
      {
        break;
      }
    }

    TTBound bound = TTBound::EXACT;
    if (best_score <= alpha_original)
    {
      bound = TTBound::UPPER;
    }
    else if (best_score >= beta)
    {
      bound = TTBound::LOWER;
    }
    store_transposition(hash_key, depth, best_score, bound, best_move);

    return best_score;
  }

  /// @brief Extend search on tactical capture sequences.
  int ChessEngine::quiescence_search(Grid& grid, int alpha, int beta, uint64_t hash_key)
  {
    nodes_searched_current_++;
    if (should_stop_search())
    {
      return evaluate_position(grid, grid.current_turn);
    }

    int static_score = evaluate_position(grid, grid.current_turn);

    if (static_score >= beta)
    {
      return beta;
    }

    if (static_score > alpha)
    {
      alpha = static_score;
    }

    // Search only captures and checks (simplified)
    auto legal_moves = get_legal_moves(grid);

    for (const auto& move : legal_moves)
    {
      if (should_stop_search())
      {
        break;
      }

      // Check if move is a capture (simplified check)
      auto target_piece = grid.get_piece(move.end_pos);
      if (!target_piece.has_value())
      {
        continue; // Skip non-captures
      }

      // Apply move in-place.
      UndoRecord undo = grid.apply_move_inplace(move);
      const uint64_t child_hash = update_zobrist_hash_for_move(hash_key, move, undo, grid);

      int score = -quiescence_search(grid, -beta, -alpha, child_hash);

      // Undo move to restore state.
      grid.undo_move(move, undo);

      if (score >= beta)
      {
        return beta;
      }

      if (score > alpha)
      {
        alpha = score;
      }
    }

    return alpha;
  }

  /// @brief Evaluate material.
  int ChessEngine::evaluate_material(const Grid& grid)
  {
    int white_material = 0;
    int black_material = 0;

    auto pieces = grid.get_all_pieces();

    for (const auto& piece : pieces)
    {
      if (piece) // Check for null pointer
      {
        int value = get_piece_value(piece->get_type());

        if (piece->get_color() == Color::WHITE)
        {
          white_material += value;
        }
        else
        {
          black_material += value;
        }
      }
    }

    return white_material - black_material;
  }

  /// @brief Evaluate positional.
  int ChessEngine::evaluate_positional(const Grid& grid)
  {
    int score = 0;

    auto pieces = grid.get_all_pieces();

    for (const auto& piece : pieces)
    {
      if (!piece)
        continue; // Skip null pointers

      int      positional_value = 0;
      Position pos              = piece->get_position();

      // Simplified positional evaluation
      switch (piece->get_type())
      {
      case PieceType::PAWN:
        // Center pawns are valuable
        if (pos.file >= 3 && pos.file <= 4)
        {
          positional_value += 20;
        }
        // Passed pawns (simplified)
        if (piece->get_color() == Color::WHITE && pos.rank > 4)
        {
          positional_value += 30;
        }
        else if (piece->get_color() == Color::BLACK && pos.rank < 3)
        {
          positional_value += 30;
        }
        break;

      case PieceType::KNIGHT:
        // Knights are good in center
        if (pos.file >= 2 && pos.file <= 5 && pos.rank >= 2 && pos.rank <= 5)
        {
          positional_value += 30;
        }
        break;

      case PieceType::BISHOP:
        // Bishops are good on long diagonals
        if ((pos.file + pos.rank) % 2 == 0)
        {
          positional_value += 10;
        }
        break;

      case PieceType::ROOK:
        // Rooks are good on open files (simplified)
        positional_value += 10;
        break;

      case PieceType::QUEEN:
        // Queens are powerful in center
        if (pos.file >= 2 && pos.file <= 5 && pos.rank >= 2 && pos.rank <= 5)
        {
          positional_value += 20;
        }
        break;

      case PieceType::KING:
        // King safety is handled separately
        break;
      }

      if (piece->get_color() == Color::WHITE)
      {
        score += positional_value;
      }
      else
      {
        score -= positional_value;
      }
    }

    return score;
  }

  /// @brief Evaluate king safety.
  int ChessEngine::evaluate_king_safety(const Grid& grid, Color color)
  {
    int safety_score = 0;

    // Find king position
    Position king_pos;
    bool     king_found = false;

    auto pieces = grid.get_all_pieces();
    for (const auto& piece : pieces)
    {
      if (piece->get_type() == PieceType::KING && piece->get_color() == color)
      {
        king_pos   = piece->get_position();
        king_found = true;
        break;
      }
    }

    if (!king_found)
    {
      return -1000; // King not found (shouldn't happen)
    }

    // Simplified king safety evaluation
    // Penalize exposed king
    if (color == Color::WHITE)
    {
      if (king_pos.rank < 2)
      {
        safety_score += 50; // King on back rank is safer
      }
      else
      {
        safety_score -= (king_pos.rank - 1) * 20; // Penalize forward king
      }
    }
    else
    {
      if (king_pos.rank > 5)
      {
        safety_score += 50; // King on back rank is safer
      }
      else
      {
        safety_score -= (6 - king_pos.rank) * 20; // Penalize forward king
      }
    }

    return safety_score;
  }

  /// @brief Get piece value.
  int ChessEngine::get_piece_value(PieceType type) const
  {
    switch (type)
    {
    case PieceType::PAWN:
      return 100;
    case PieceType::KNIGHT:
      return 320;
    case PieceType::BISHOP:
      return 330;
    case PieceType::ROOK:
      return 500;
    case PieceType::QUEEN:
      return 900;
    case PieceType::KING:
      return 20000;
    default:
      return 0;
    }
  }

  /// @brief Return whether time limit exceeded.
  bool ChessEngine::is_time_limit_exceeded(const std::chrono::steady_clock::time_point& start_time,
                                           const SearchLimits&                          limits) const
  {
    if (limits.infinite)
    {
      return false;
    }

    auto current_time = std::chrono::steady_clock::now();
    auto elapsed      = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);

    return elapsed >= limits.max_time;
  }

  /// @brief Implement should stop search.
  bool ChessEngine::should_stop_search() const
  {
    if (nodes_searched_current_ >= current_limits_.max_nodes)
    {
      return true;
    }
    return is_time_limit_exceeded(search_start_time_, current_limits_);
  }

  /// @brief Compute position hash.
  uint64_t ChessEngine::compute_position_hash(const Grid& grid) const
  {
    uint64_t hash = 0;

    for (int file = 0; file < 8; ++file)
    {
      for (int rank = 0; rank < 8; ++rank)
      {
        const Position pos(file, rank);
        const auto piece = grid.get_piece(pos);
        if (!piece.has_value())
        {
          continue;
        }

        const size_t color_index = (piece->color == Color::WHITE) ? 0 : 1;
        const size_t type_index = static_cast<size_t>(piece->type);
        const size_t square_index = static_cast<size_t>(pos.rank * 8 + pos.file);
        hash ^= zobrist_piece_table_[color_index][type_index][square_index];
      }
    }

    if (grid.current_turn == Color::BLACK)
    {
      hash ^= zobrist_side_to_move_;
    }

    if (grid.flags.white_can_castle_kingside)
      hash ^= zobrist_castling_rights_[0];
    if (grid.flags.white_can_castle_queenside)
      hash ^= zobrist_castling_rights_[1];
    if (grid.flags.black_can_castle_kingside)
      hash ^= zobrist_castling_rights_[2];
    if (grid.flags.black_can_castle_queenside)
      hash ^= zobrist_castling_rights_[3];

    return hash;
  }

  /// @brief Update zobrist hash for move.
  uint64_t ChessEngine::update_zobrist_hash_for_move(uint64_t parent_hash,
                                                      const Move& move,
                                                      const UndoRecord& undo,
                                                      const Grid& grid_after_move) const
  {
    uint64_t hash = parent_hash;
    const Color moving_color = undo.turn_before;

    auto xor_piece = [this, &hash](PieceType type, Color color, const Position& pos)
    {
      if (!pos.is_valid())
      {
        return;
      }
      const size_t color_index = (color == Color::WHITE) ? 0 : 1;
      const size_t type_index = static_cast<size_t>(type);
      const size_t square_index = static_cast<size_t>(pos.rank * 8 + pos.file);
      hash ^= zobrist_piece_table_[color_index][type_index][square_index];
    };

    auto toggle_if_changed = [&hash](bool before, bool after, uint64_t key)
    {
      if (before != after)
      {
        hash ^= key;
      }
    };

    const PieceType moved_type_before =
        move.is_castling() ? PieceType::KING : (move.is_promotion() || move.is_en_passant() ? PieceType::PAWN
                                                                                              : grid_after_move.get_piece(move.end_pos)->type);
    const PieceType moved_type_after = move.is_promotion() ? move.promotion_piece : moved_type_before;

    // Remove mover from start square, add mover on end square.
    xor_piece(moved_type_before, moving_color, move.start_pos);
    xor_piece(moved_type_after, moving_color, move.end_pos);

    // Remove captured piece(s) from hash.
    if (undo.captured_piece.has_value())
    {
      xor_piece(undo.captured_piece->type, undo.captured_piece->color, move.end_pos);
    }
    if (undo.en_passant_pawn.has_value())
    {
      const Position captured_pos(move.end_pos.file, move.start_pos.rank);
      xor_piece(undo.en_passant_pawn->type, undo.en_passant_pawn->color, captured_pos);
    }

    // Castling rook displacement.
    if (move.is_castling())
    {
      const Position rook_start = undo.rook_original_pos;
      const Position rook_end = (move.flags == SpecialFlags::CASTLE_KINGSIDE)
                                    ? Position(5, move.start_pos.rank)
                                    : Position(3, move.start_pos.rank);
      xor_piece(PieceType::ROOK, moving_color, rook_start);
      xor_piece(PieceType::ROOK, moving_color, rook_end);
    }

    // Side-to-move flips every ply.
    hash ^= zobrist_side_to_move_;

    toggle_if_changed(undo.flags_before.white_can_castle_kingside,
                      grid_after_move.flags.white_can_castle_kingside,
                      zobrist_castling_rights_[0]);
    toggle_if_changed(undo.flags_before.white_can_castle_queenside,
                      grid_after_move.flags.white_can_castle_queenside,
                      zobrist_castling_rights_[1]);
    toggle_if_changed(undo.flags_before.black_can_castle_kingside,
                      grid_after_move.flags.black_can_castle_kingside,
                      zobrist_castling_rights_[2]);
    toggle_if_changed(undo.flags_before.black_can_castle_queenside,
                      grid_after_move.flags.black_can_castle_queenside,
                      zobrist_castling_rights_[3]);

    return hash;
  }

  /// @brief Probe transposition.
  bool ChessEngine::probe_transposition(uint64_t key,
                                        int depth,
                                        int alpha,
                                        int beta,
                                        int& score_out,
                                        Move* best_move_out) const
  {
    auto it = transposition_table_.find(key);
    if (it == transposition_table_.end())
    {
      return false;
    }

    const TTEntry& entry = it->second;
    if (best_move_out != nullptr)
    {
      *best_move_out = entry.best_move;
    }

    if (entry.depth < depth)
    {
      return false;
    }

    switch (entry.bound)
    {
    case TTBound::EXACT:
      score_out = entry.score;
      return true;
    case TTBound::LOWER:
      if (entry.score >= beta)
      {
        score_out = entry.score;
        return true;
      }
      break;
    case TTBound::UPPER:
      if (entry.score <= alpha)
      {
        score_out = entry.score;
        return true;
      }
      break;
    }

    return false;
  }

  /// @brief Store transposition.
  void ChessEngine::store_transposition(uint64_t key,
                                        int depth,
                                        int score,
                                        TTBound bound,
                                        const Move& best_move)
  {
    TTEntry& entry = transposition_table_[key];
    if (entry.depth > depth)
    {
      return;
    }

    entry.depth = depth;
    entry.score = score;
    entry.bound = bound;
    entry.best_move = best_move;
  }

} // namespace Chess


