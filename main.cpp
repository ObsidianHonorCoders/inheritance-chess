/// ---------------------------------------------------------------------------------
/// @file      main.cpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Basic Chess Engine demonstrating C++ inheritance and polymorphism.
/// @version   1.0
/// @date      2025-12-30
/// @copyright MIT License - see LICENSE file for details
/// ---------------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <memory>

#include "board.hpp"

int main()
{
  std::cout << "====================================" << std::endl;
  std::cout << "|  Inheritance Chess Engine v1.0   |" << std::endl;
  std::cout << "====================================" << std::endl;
  std::cout << "Starting Inheritance Chess!" << std::endl;

  Pawn white_pawn('e', '2', PieceColor::WHITE);
  Pawn black_pawn('d', '7', PieceColor::BLACK);

  Board chessBoard;
  chessBoard.initializeStandardSetup();

  std::cout << "Standard Chess Starting Position:" << std::endl;
  chessBoard.display();

  return 0;
}
