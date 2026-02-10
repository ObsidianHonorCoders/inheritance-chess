/// @file         bishop.hpp
/// @brief        Bishop piece implementation for the chess engine.
/// @project      Inheritance Chess
/// @organization Obsidian Honor Coders
/// @repository   https://github.com/ObsidianHonorCoders/inheritance-chess
/// @author       Calileus
/// @details      Defines the Bishop class that inherits from Piece and implements
///               bishop-specific movement rules for the chess game.
/// @date         2026-02-06
/// @copyright    2026 Obsidian Honor Coders
/// @license      Apache License 2.0 - http://www.apache.org/licenses/LICENSE-2.0
/// @note         Licensed under the Apache License, Version 2.0 (the "License");
///               you may not use this file except in compliance with the License.

#ifndef ICHESS_SRC_BISHOP
#define ICHESS_SRC_BISHOP

#include "pieces.hpp"

/// @class   Bishop
/// @brief   Represents a Bishop chess piece.
/// @details Inherits from Piece and implements bishop-specific movement rules.
///          Bishops move diagonally any number of squares.
class Bishop : public Piece
{
  public:
    /// @brief Construct a Bishop at the specified position and color.
    /// @param file The file (column) coordinate ('a' to 'h').
    /// @param rank The rank (row) coordinate ('1' to '8').
    /// @param color The color of the bishop (WHITE or BLACK).
    Bishop(char file, char rank, PieceColor color) : Piece(color, PieceType::BISHOP) { set_position(file, rank); }

    /// @brief Virtual destructor for proper cleanup.
    virtual ~Bishop() override {};

    /// @brief      Calculate valid moves for this bishop.
    /// @param[out] p     Vector to be filled with valid move positions.
    /// @param[in]  other Vector of pointers to all other pieces on the board for move validation.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Bishops move diagonally without restriction.
    virtual void moves(std::vector<PiecePosition>& p, const PieceList other) const override;

    /// @brief      Calculate valid moves for this bishop.
    /// @param[out] p        Vector to be filled with valid move positions.
    /// @param[in]  other_p  Vector of positions of all other pieces on the board for move validation.
    /// @param[in]  other_c  Vector of colors corresponding to each piece in other_p for determining valid captures.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Bishops move diagonally without restriction.
    /// @details    This overload provides piece positions and colors separately for move calculation.
    virtual void moves(std::vector<PiecePosition>&      p,
                       const std::vector<PiecePosition> other_p,
                       const std::vector<PieceColor>    other_c) const override;
};

#endif // ICHESS_SRC_BISHOP
