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
/// @details Initializes the chess board with the standard starting position
///          and displays it to the console.
int main()
{
  std::cout << "====================================" << std::endl;
  std::cout << "|  Inheritance Chess Engine v1.0   |" << std::endl;
  std::cout << "====================================" << std::endl;
  std::cout << "Starting Inheritance Chess!" << std::endl;

  Board chessBoard;
  chessBoard.initializeStandardSetup();

  std::cout << "Standard Chess Starting Position:" << std::endl;
  chessBoard.display();

  return 0;
}
