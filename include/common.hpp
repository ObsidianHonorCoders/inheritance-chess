/// ---------------------------------------------------------------------------------
/// @file      common.hpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Basic Chess Engine demonstrating C++ inheritance and polymorphism.
/// @version   1.0
/// @date      2026-01-27
/// @copyright MIT License - see LICENSE file for details
/// ---------------------------------------------------------------------------------
#ifndef ICHESS_SRC_COMMON
#define ICHESS_SRC_COMMON

enum class PieceColor : char
{
  NONE  = ' ',
  WHITE = 'w',
  BLACK = 'b'
};

enum class PieceRepresentation : char
{
  NONE   = ' ',
  PAWN   = 'P',
  KNIGHT = 'N',
  BISHOP = 'B',
  ROOK   = 'R',
  QUEEN  = 'Q',
  KING   = 'K'
};

struct PiecePosition
{
    char file; // e.g., 'a' through 'h'
    char rank; // e.g., '1' through '8'
};

#endif // ICHESS_SRC_COMMON
