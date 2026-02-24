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

/// @brief      Calculate valid moves for this piece using List parameter.
/// @param[out] p     Vector to be filled with valid move positions.
/// @param[in]  other Vector of unique pointers to all other pieces on the board for move validation.
/// @details    This method extracts positions and colors from the List and calls the pure virtual overload.
///             Provides a convenient interface for callers using the Piece::List container.
void Piece::available_moves(PositionList& p, const List& other) const
{
  PositionList other_p;
  ColorList    other_c;
  char         f, r;
  for (const std::unique_ptr<Piece>& piece : other)
  {
    piece->get_position(f, r);
    other_p.push_back({f, r});

    if (piece->is_white())
    {
      other_c.push_back(Piece::Color::WHITE);
    }
    else if (piece->is_black())
    {
      other_c.push_back(Piece::Color::BLACK);
    }
    else
    {
      other_c.push_back(Piece::Color::NONE);
    }
  }
  available_moves(p, other_p, other_c);
}
