/// @file      main.cpp
/// @brief     Entry point for Inheritance ChessEngine application.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Demonstrates advanced chess engine functionality including AI,
///            move evaluation, search algorithms, and modular architecture.

#include <iostream>
#include <string>
#include <memory>

#include "cgh/include/game_handler.h"
#include "cei/include/chess_engine.h"

int main()
{
  // Initialize the complete chess game handler with all modules
  Chess::ChessGameHandler gameHandler;

  // Initialize the chess engine AI
  Chess::ChessEngine chessEngine;

  std::cout << "====================================\n";
  std::cout << "|  Inheritance Chess Engine v0.0   |\n";
  std::cout << "====================================\n";
  std::cout << "Starting Inheritance Chess with AI!\n\n";

  // Start a new game
  gameHandler.start_new_game();
  std::cout << "New game started with standard position.\n\n";

  // Display initial board
  gameHandler.display_board();

  // Demonstrate AI move finding
  std::cout << "\nDemonstrating AI move analysis:\n";
  std::cout << "AI is analyzing the position...\n";

  // Set up search limits for AI
  Chess::SearchLimits limits;
  limits.max_depth = 3;
  limits.max_time  = std::chrono::milliseconds(1000);

  // Find best move using AI
  auto aiResult = chessEngine.find_best_move(gameHandler.get_current_grid(), limits);

  std::cout << "AI Analysis Results:\n";
  std::cout << "Best move found: (" << aiResult.best_move.start_pos.file << "," << aiResult.best_move.start_pos.rank
            << ") -> (" << aiResult.best_move.end_pos.file << "," << aiResult.best_move.end_pos.rank << ")\n";
  std::cout << "Evaluation score: " << aiResult.score << " centipawns\n";
  std::cout << "Search depth: " << aiResult.depth << "\n";
  std::cout << "Nodes searched: " << aiResult.nodes_searched << "\n";
  std::cout << "Time used: " << aiResult.time_used.count() << " ms\n\n";

  // Demonstrate position evaluation
  std::cout << "Position Evaluation:\n";
  int whiteEvaluation = chessEngine.evaluate_position(gameHandler.get_current_grid(), Chess::Color::WHITE);
  int blackEvaluation = chessEngine.evaluate_position(gameHandler.get_current_grid(), Chess::Color::BLACK);

  std::cout << "White perspective: " << whiteEvaluation << " centipawns\n";
  std::cout << "Black perspective: " << blackEvaluation << " centipawns\n";

  if (whiteEvaluation > 0)
  {
    std::cout << "White has a slight advantage\n";
  }
  else if (whiteEvaluation < 0)
  {
    std::cout << "Black has a slight advantage\n";
  }
  else
  {
    std::cout << "Position is roughly equal\n";
  }

  // Demonstrate engine difficulty settings
  std::cout << "\nTesting engine difficulty levels:\n";
  chessEngine.set_difficulty(3); // Easy
  std::cout << "Set difficulty to Easy (3/10)\n";

  auto easyResult = chessEngine.find_best_move(gameHandler.get_current_grid(), limits);
  std::cout << "Easy mode - Score: " << easyResult.score << ", Depth: " << easyResult.depth << "\n";

  chessEngine.set_difficulty(8); // Hard
  std::cout << "Set difficulty to Hard (8/10)\n";

  auto hardResult = chessEngine.find_best_move(gameHandler.get_current_grid(), limits);
  std::cout << "Hard mode - Score: " << hardResult.score << ", Depth: " << hardResult.depth << "\n";

  // Show engine statistics
  std::cout << "\n" << chessEngine.get_statistics() << "\n";

  // Demonstrate legal moves with AI analysis
  std::cout << "Analyzing all legal moves:\n";
  auto legalMoves = gameHandler.get_legal_moves();
  std::cout << "Found " << legalMoves.size() << " legal moves\n";

  // Show top 3 moves (simplified)
  if (legalMoves.size() >= 3)
  {
    std::cout << "Top 3 moves to consider:\n";
    for (int i = 0; i < 3 && i < legalMoves.size(); i++)
    {
      const auto& move = legalMoves[i];
      std::cout << "  " << i + 1 << ". (" << move.start_pos.file << "," << move.start_pos.rank << ") -> ("
                << move.end_pos.file << "," << move.end_pos.rank << ")\n";
    }
  }

  // Test draw detection
  std::cout << "\nDraw detection test:\n";
  bool isDraw = chessEngine.is_draw(gameHandler.get_current_grid());
  std::cout << "Current position is " << (isDraw ? "a draw" : "not a draw") << "\n";

  std::cout << "\nThanks for using our software!\n";
  std::cout << "                               - OHC team.\n";
  std::cout << "====================================\n";

  return 0;
}
