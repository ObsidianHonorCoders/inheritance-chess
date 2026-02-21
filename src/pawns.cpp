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

#include "pawns.hpp"

/// @brief      Calculate valid moves for this pawn.
/// @param[out] p      Vector to be filled with valid move positions.
/// @param[in]  other_p  Vector of positions of all other pieces on the board for move validation.
/// @param[in]  other_c  Vector of colors corresponding to each piece in oth_p for determining valid captures.
/// @throws     std::runtime_error if the piece has an invalid color.
/// @note       Implementation distinguishes between white and black pawns for directional movement.
/// @details    This overload provides piece positions and colors separately for move calculation.
void Pawn::moves(Piece::PositionList& p, const Piece::PositionList other_p, const Piece::ColorList other_c) const {}
