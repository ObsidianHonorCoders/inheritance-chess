/// @file         pawns.cpp
/// @brief        Implementation of the Pawn chess piece class.
/// @project      Inheritance Chess
/// @organization Obsidian Honor Coders
/// @author       Calileus
/// @details      Provides Pawn-specific functionality and move calculation.
///               Currently serves as a placeholder for future pawn movement logic.
/// @date         2026-01-01
/// @copyright    2026 Obsidian Honor Coders
/// @license      Apache License 2.0 - http://www.apache.org/licenses/LICENSE-2.0
/// @note         Licensed under the Apache License, Version 2.0 (the "License");
///               you may not use this file except in compliance with the License.

#include <iostream>
#include <algorithm>

#include "pawns.hpp"

static const int get_direction(const Piece::Color c)
{
  int direction = 0;
  switch (c)
  {
  case Piece::Color::WHITE:
    direction = 1; // White pawns move up (increase rank)
    break;
  case Piece::Color::BLACK:
    direction = -1; // Black pawns move down (decrease rank)
  default:
    throw std::runtime_error("Invalid pawn color");
    break;
  }
  return direction;
}

static const bool try_to_capture(const Piece::PositionList& other_p,
                                 const Piece::ColorList&    other_c,
                                 const Position&            target_p,
                                 const Piece::Color         my_color)
{
  Piece::PositionList::const_iterator found_piece_itera  = std::find(other_p.begin(), other_p.end(), target_p);
  size_t                              found_piece_index  = 0;
  bool                                valid_capture_flag = false;
  if (found_piece_itera != other_p.end())
  {
    found_piece_index = std::distance(other_p.begin(), found_piece_itera);
    if (other_c[found_piece_index] != my_color)
    {
      valid_capture_flag = true;
    }
  }
  return valid_capture_flag;
}

static const bool try_to_capture_passant(const Properties& props, const Position& target_p, const int direct)
{
  bool     valid_capture_flag   = false;
  Position piece_to_capture     = target_p;
  Position piece_to_capture_pre = target_p;
  piece_to_capture.rank -= direct;
  piece_to_capture_pre.rank += direct;
  if (props.turns_since_pawn_move == 0 && props.last_move_end == piece_to_capture
      && props.last_move_start == piece_to_capture_pre)
  {
    valid_capture_flag = true;
  }
  return valid_capture_flag;
}

/// @brief      Calculate valid moves for this pawn.
/// @param[out] p       Vector to be filled with valid move positions.
/// @param[in]  other_p Vector of positions of all other pieces on the board for move validation.
/// @param[in]  other_c Vector of colors corresponding to each piece in oth_p for determining valid captures.
/// @param[in]  props   Properties of the board for move validation, used for en passant.
/// @throws     std::runtime_error if the piece has an invalid color.
/// @note       Implementation distinguishes between white and black pawns for directional movement.
/// @details    This overload provides piece positions and colors separately for move calculation.
void Pawn::available_moves(Piece::PositionList&       p,
                           const Piece::PositionList& other_p,
                           const Piece::ColorList&    other_c,
                           const Properties&          props) const
{
  int      direction      = 0;
  Position move_candidate = position;

  p.clear();
  direction = get_direction(color);

  // Try to move one square forward if you are still in the board and the place is empty.
  move_candidate = {position.file, static_cast<char>(position.rank + direction)};
  if (is_in_grid_range(move_candidate) && other_p.end() == std::find(other_p.begin(), other_p.end(), move_candidate))
  {
    p.push_back(move_candidate);

    // Try to move two squares forward if you are still in the starting position and the place is empty.
    move_candidate = {position.file, static_cast<char>(position.rank + 2 * direction)};
    if (((position.rank == '2' && color == Piece::Color::WHITE)
         || (position.rank == '7' && color == Piece::Color::BLACK))
        && is_in_grid_range(move_candidate)
        && other_p.end() == std::find(other_p.begin(), other_p.end(), move_candidate))
    {
      p.push_back(move_candidate);
    }
  }

  // Capture diagonally left
  move_candidate = {static_cast<char>(position.file - 1), static_cast<char>(position.rank + direction)};
  if (is_in_grid_range(move_candidate) && try_to_capture(other_p, other_c, move_candidate, color))
  {
    p.push_back(move_candidate);
  }

  // Capture diagonally right
  move_candidate = {static_cast<char>(position.file + 1), static_cast<char>(position.rank + direction)};
  if (is_in_grid_range(move_candidate) && try_to_capture(other_p, other_c, move_candidate, color))
  {
    p.push_back(move_candidate);
  }

  // En passant capture left
  move_candidate = {static_cast<char>(position.file - 1), static_cast<char>(position.rank + direction)};
  if (is_in_grid_range(move_candidate) && try_to_capture_passant(props, move_candidate, direction))
  {
    p.push_back(move_candidate);
  }

  // En passant capture right
  move_candidate = {static_cast<char>(position.file + 1), static_cast<char>(position.rank + direction)};
  if (is_in_grid_range(move_candidate) && try_to_capture_passant(props, move_candidate, direction))
  {
    p.push_back(move_candidate);
  }
}
