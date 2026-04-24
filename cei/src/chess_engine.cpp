/// @file      chess_engine.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Engine Interface (CEI).
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements move evaluation, search algorithms, and AI capabilities.

#include "chess_engine.h"
#include <algorithm>
#include <iostream>
#include <thread>
#include <chrono>

namespace Chess
{

  ChessEngine::ChessEngine()
      : game_handler_(std::make_unique<ChessGameHandler>()),
        difficulty_level_(5),
        total_nodes_searched_(0),
        total_search_time_(0),
        searches_performed_(0)
  {
  }

  EvaluationResult ChessEngine::find_best_move(const Grid& grid, const SearchLimits& limits)
  {
    EvaluationResult result;
    result.depth = 0;
    result.nodes_searched = 0;
    result.score = 0;
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Get legal moves for current position
    auto legal_moves = get_legal_moves(grid);
    
    if (legal_moves.empty())
    {
      // No legal moves - game over
      result.score = (grid.current_turn == Color::WHITE) ? -10000 : 10000;
      return result;
    }
    
    // Adjust search depth based on difficulty
    int max_depth = std::min(limits.max_depth, difficulty_level_ + 3);
    
    // Initialize best move
    result.best_move = legal_moves[0];
    int best_score = -99999;
    
    // Search each legal move
    for (const auto& move : legal_moves)
    {
      // For now, just evaluate current position without move execution
      // TODO: Implement proper move execution in temporary grid
      
      int score = minimax(grid, max_depth - 1, -99999, 99999, false, limits);
      result.nodes_searched++;
      
      if (score > best_score)
      {
        best_score = score;
        result.best_move = move;
      }
      
      // Check time limit
      if (is_time_limit_exceeded(start_time, limits))
      {
        break;
      }
    }
    
    result.score = best_score;
    result.depth = max_depth;
    
    auto end_time = std::chrono::steady_clock::now();
    result.time_used = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Update statistics
    total_nodes_searched_ += result.nodes_searched;
    total_search_time_ += result.time_used;
    searches_performed_++;
    
    return result;
  }

  int ChessEngine::evaluate_position(const Grid& grid, Color color)
  {
    int material_score = evaluate_material(grid);
    int positional_score = evaluate_positional(grid);
    int king_safety_score = evaluate_king_safety(grid, color) - evaluate_king_safety(grid, (color == Color::WHITE) ? Color::BLACK : Color::WHITE);
    
    int total_score = material_score + positional_score + king_safety_score;
    
    // Adjust score based on perspective
    if (color == Color::BLACK)
    {
      total_score = -total_score;
    }
    
    return total_score;
  }

  std::vector<Move> ChessEngine::get_legal_moves(const Grid& grid)
  {
    // Use game handler to get legal moves
    return game_handler_->get_legal_moves(grid);
  }

  bool ChessEngine::is_draw(const Grid& grid)
  {
    // Use game handler to check for draw
    // Note: We need to check the grid state, not the game handler's internal state
    // For now, use a simple check - TODO: implement proper draw detection
    return false; // Default to not a draw
  }

  void ChessEngine::set_difficulty(int difficulty)
  {
    difficulty_level_ = std::clamp(difficulty, 1, 10);
  }

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
      uint64_t avg_time = total_search_time_.count() / searches_performed_;
      stats += "Average Nodes per Search: " + std::to_string(avg_nodes) + "\n";
      stats += "Average Time per Search: " + std::to_string(avg_time) + " ms\n";
    }
    
    return stats;
  }

  void ChessEngine::reset_statistics()
  {
    total_nodes_searched_ = 0;
    total_search_time_ = std::chrono::milliseconds(0);
    searches_performed_ = 0;
  }

  int ChessEngine::minimax(const Grid& grid, int depth, int alpha, int beta, bool maximizing, 
                           const SearchLimits& limits)
  {
    // Base case: leaf node
    if (depth == 0)
    {
      return quiescence_search(grid, alpha, beta);
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
    
    for (const auto& move : legal_moves)
    {
      // For now, just evaluate current position without move execution
      // TODO: Implement proper move execution in temporary grid
      
      int score = minimax(grid, depth - 1, alpha, beta, !maximizing, limits);
      
      if (maximizing)
      {
        best_score = std::max(best_score, score);
        alpha = std::max(alpha, score);
      }
      else
      {
        best_score = std::min(best_score, score);
        beta = std::min(beta, score);
      }
      
      // Alpha-beta pruning
      if (beta <= alpha)
      {
        break;
      }
    }
    
    return best_score;
  }

  int ChessEngine::quiescence_search(const Grid& grid, int alpha, int beta)
  {
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
      // Check if move is a capture (simplified check)
      auto target_piece = grid.get_piece(move.end_pos);
      if (!target_piece.has_value())
      {
        continue; // Skip non-captures
      }
      
      // For now, just evaluate current position without move execution
      // TODO: Implement proper move execution in temporary grid
      int score = -quiescence_search(grid, -beta, -alpha);
      
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

  int ChessEngine::evaluate_positional(const Grid& grid)
  {
    int score = 0;
    
    auto pieces = grid.get_all_pieces();
    
    for (const auto& piece : pieces)
    {
      if (!piece) continue; // Skip null pointers
      
      int positional_value = 0;
      Position pos = piece->get_position();
      
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

  int ChessEngine::evaluate_king_safety(const Grid& grid, Color color)
  {
    int safety_score = 0;
    
    // Find king position
    Position king_pos;
    bool king_found = false;
    
    auto pieces = grid.get_all_pieces();
    for (const auto& piece : pieces)
    {
      if (piece->get_type() == PieceType::KING && piece->get_color() == color)
      {
        king_pos = piece->get_position();
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

  int ChessEngine::get_piece_value(PieceType type) const
  {
    switch (type)
    {
      case PieceType::PAWN:   return 100;
      case PieceType::KNIGHT: return 320;
      case PieceType::BISHOP: return 330;
      case PieceType::ROOK:   return 500;
      case PieceType::QUEEN:  return 900;
      case PieceType::KING:   return 20000;
      default:                return 0;
    }
  }

  bool ChessEngine::is_time_limit_exceeded(const std::chrono::steady_clock::time_point& start_time,
                                           const SearchLimits& limits) const
  {
    if (limits.infinite)
    {
      return false;
    }
    
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
    
    return elapsed >= limits.max_time;
  }

} // namespace Chess
