/// @file         knight.hpp
/// @brief        Knight piece implementation for the chess engine.
/// @project      Inheritance Chess
/// @organization Obsidian Honor Coders
/// @repository   https://github.com/ObsidianHonorCoders/inheritance-chess
/// @author       Calileus
/// @details      Defines the Knight class that inherits from Piece and implements
///               knight-specific movement rules for the chess game.
/// @date         2026-02-06
/// @copyright    2026 Obsidian Honor Coders
/// @license      Apache License 2.0 - http://www.apache.org/licenses/LICENSE-2.0
/// @note         Licensed under the Apache License, Version 2.0 (the "License");
///               you may not use this file except in compliance with the License.

#ifndef ICHESS_SRC_KNIGHT
#define ICHESS_SRC_KNIGHT

#include "pieces.hpp"

/// @class   Knight
/// @brief   Represents a Knight chess piece.
/// @details Inherits from Piece and implements knight-specific movement rules.
///          Knights move in an L-shape: two squares in one direction and one square perpendicular.
class Knight : public Piece
{
  public:
    /// @brief Construct a Knight at the specified position and color.
    /// @param file The file (column) coordinate ('a' to 'h').
    /// @param rank The rank (row) coordinate ('1' to '8').
    /// @param color The color of the knight (WHITE or BLACK).
    Knight(char file, char rank, Piece::Color color) : Piece(color, Piece::Type::KNIGHT) { set_position(file, rank); }

    /// @brief Virtual destructor for proper cleanup.
    virtual ~Knight() override {};

    /// @brief      Calculate valid moves for this knight.
    /// @param[out] p       Vector to be filled with valid move positions.
    /// @param[in]  other_p Vector of positions of all other pieces on the board for move validation.
    /// @param[in]  other_c Vector of colors corresponding to each piece in other_p for determining valid captures.
    /// @param[in]  props   Properties of the board for move validation, unused in Knigth moves.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Knights move in an L-shape pattern, regardless of other pieces on the board.
    /// @details    This overload provides piece positions and colors separately for move calculation.
    virtual void available_moves(Piece::PositionList&       p,
                                 const Piece::PositionList& other_p,
                                 const Piece::ColorList&    other_c,
                                 const Properties&          props = default_properties) const override;
};

#endif // ICHESS_SRC_KNIGHT
