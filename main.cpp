/// @file      main.cpp
/// @brief     Entry point for Inheritance ChessEngine application.
/// @author    Calileus
/// @date      2025-12-30
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Demonstrates basic chess engine functionality including board setup,
///            piece creation, and console-based board display using C++ inheritance

#include <iostream>
#include <string>
#include <memory>

#include "cgh/include/game_handler.h"

int main()
{
  // Initialize the complete chess game handler with all modules
  Chess::ChessGameHandler gameHandler;
  
  std::cout << "====================================\n";
  std::cout << "|  Inheritance Chess Engine v0.0   |\n";
  std::cout << "====================================\n";
  std::cout << "Starting Inheritance Chess!\n\n";
  
  // Start a new game
  gameHandler.start_new_game();
  std::cout << "New game started with standard position.\n\n";
  
  // Display initial board
  gameHandler.display_board();
  
  // Demonstrate move making
  std::cout << "\nDemonstrating move execution:\n";
  std::cout << "Trying move e2-e4...\n";
  
  if (gameHandler.make_move("e2e4"))
  {
    std::cout << "Move successful!\n";
    gameHandler.display_board();
  }
  else
  {
    std::cout << "Move failed!\n";
  }
  
  // Show current FEN
  std::cout << "\nCurrent FEN: " << gameHandler.get_current_fen() << "\n";
  
  // Show game state
  std::cout << "Game state: ";
  switch (gameHandler.get_game_state())
  {
    case Chess::GameState::ONGOING:
      std::cout << "Ongoing\n";
      break;
    case Chess::GameState::CHECKMATE:
      std::cout << "Checkmate\n";
      break;
    case Chess::GameState::STALEMATE:
      std::cout << "Stalemate\n";
      break;
    case Chess::GameState::DRAW_INSUFFICIENT_MATERIAL:
    case Chess::GameState::DRAW_THREEFOLD_REPETITION:
    case Chess::GameState::DRAW_FIFTY_MOVE_RULE:
      std::cout << "Draw\n";
      break;
    default:
      std::cout << "Unknown\n";
      break;
  }
  
  // Demonstrate legal moves
  std::cout << "\nGetting legal moves for current position...\n";
  auto legalMoves = gameHandler.get_legal_moves();
  std::cout << "Found " << legalMoves.size() << " legal moves\n";
  
  std::cout << "\nThanks for using our software!\n";
  std::cout << "                               - OHC team.\n";
  std::cout << "====================================\n";
  
  return 0;
}
