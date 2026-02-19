/// @file         pieces.cpp
/// @brief        Implementation of the base Piece (chess piece) class.
/// @project      Inheritance Chess
/// @organization Obsidian Honor Coders
/// @author       Calileus
/// @details      Provides implementations for color checking, position management,
///               and piece representation functionality.
/// @date         2026-01-01
/// @copyright    2026 Obsidian Honor Coders
/// @license      Apache License 2.0 - http://www.apache.org/licenses/LICENSE-2.0
/// @note         Licensed under the Apache License, Version 2.0 (the "License");
///               you may not use this file except in compliance with the License.

#include <cctype>

#include "pieces.hpp"

/// @brief   Default constructor for Piece.
/// @details Private to prevent instantiation without color and type specification.
Piece::Piece() {}

/// @brief   Construct a Piece with specified color and type.
/// @details Initializes the piece with the given color and type, and sets position to empty.
Piece::Piece(Piece::Color col, Piece::Type typ) : color(col), type(typ) { set_position(' ', ' '); }

/// @brief  Check if this piece is black.
/// @return True if the piece color is BLACK.
const bool Piece::is_black() const { return color == Piece::Color::BLACK; }

/// @brief  Check if this piece is white.
/// @return True if the piece color is WHITE.
const bool Piece::is_white() const { return color == Piece::Color::WHITE; }

/// @brief   Set the position of the piece on the board (file: 'a'-'h', rank: '1'-'8').
/// @details Validates coordinates and sets them if valid (file: a-h, rank: 1-8).
///          Invalid coordinates are set to space character ' '.
void Piece::set_position(const char f, const char r)
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

/// @brief      Get the current position of the piece (file: 'a'-'h', rank: '1'-'8').
/// @param[out] f Reference to store the file (column) coordinate.
/// @param[out] r Reference to store the rank (row) coordinate.
void Piece::get_position(char& f, char& r)
{
  f = position.file;
  r = position.rank;
}

/// @brief   Get the character representation of the piece.
/// @details Returns uppercase letters for white pieces and lowercase for black pieces.
/// @return  Character representation of the piece type.
const char Piece::get_representation() const
{
  char res = ' ';
  if (color == Piece::Color::WHITE)
  {
    res = static_cast<char>(type);
  }
  else if (color == Piece::Color::BLACK)
  {
    res = static_cast<char>(std::tolower(static_cast<char>(type)));
  }
  return res;
}
