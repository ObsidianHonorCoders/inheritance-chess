/// ---------------------------------------------------------------------------------
/// @file      piezas.cpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Basic Chess Engine demonstrating C++ inheritance and polymorphism.
/// @version   1.0
/// @date      2026-01-01
/// @copyright MIT License - see LICENSE file for details
/// ---------------------------------------------------------------------------------
#include "piezas.hpp"

#include <cctype>

Pieza::Pieza() {}

Pieza::Pieza(PieceColor col, PieceType typ) : color(col), type(typ) { set_position(' ', ' '); }

const bool Pieza::is_black() const { return color == PieceColor::BLACK; }
const bool Pieza::is_white() const { return color == PieceColor::WHITE; }

void Pieza::set_position(const char f, const char r)
{
  if ('a' <= f && f <= 'h')
  {
    position.file = f;
  }
  else
  {
    position.file = ' ';
  }
  if ('1' <= r && r <= '8')
  {
    position.rank = r;
  }
  else
  {
    position.rank = ' ';
  }
}

void Pieza::get_position(char& f, char& r)
{
  f = position.file;
  r = position.rank;
}

const char Pieza::get_representation() const
{
  char res = ' ';
  if (color == PieceColor::WHITE)
  {
    res = static_cast<char>(type);
  }
  else if (color == PieceColor::BLACK)
  {
    res = static_cast<char>(std::tolower(static_cast<char>(type)));
  }
  return res;
}
