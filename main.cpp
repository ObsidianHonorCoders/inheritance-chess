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

#include "pawns.hpp"

int main()
{
  std::cout << "Starting Inheritance Chess!" << std::endl;
  Pawn white_pawn('e', '2', PieceColor::WHITE);
  Pawn black_pawn('d', '7', PieceColor::BLACK);
  return 0;
}
