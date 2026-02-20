/// @file      main.cpp
/// @brief     Entry point for Inheritance ChessEngine application.
/// @author    Calileus
/// @date      2025-12-30
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Demonstrates basic chess engine functionality including board setup,
///            piece creation, and console-based board display using C++ inheritance
///            and polymorphism principles.

#include <iostream>
#include <string>
#include <memory>

#include "board.hpp"

/// @brief   Main entry point for the Inheritance Chess Engine.
/// @return  Exit status code (0 for success).
/// @details Initializes the chess board with the standard starting position.
int main()
{
  Board chessBoard;
  chessBoard.initializeStandardSetup();

  std::cout << "====================================" << std::endl;
  std::cout << "|  Inheritance Chess Engine v0.0   |" << std::endl;
  std::cout << "====================================" << std::endl;
  std::cout << "Starting Inheritance Chess!" << std::endl << std::endl;
  std::cout << "Chess Board successfully created and initialized." << std::endl;
  std::cout << "All further testing will be conducted via GTest." << std::endl;
  std::cout << "Communication protocol and usage: TBD." << std::endl << std::endl;
  std::cout << "Thanks for using our software!" << std::endl;
  std::cout << "                               - OHC team." << std::endl;

  return 0;
}
