/// @file         queen.hpp
/// @brief        Queen piece implementation for the chess engine.
/// @project      Inheritance Chess
/// @organization Obsidian Honor Coders
/// @repository   https://github.com/ObsidianHonorCoders/inheritance-chess
/// @author       Calileus
/// @details      Defines the Queen class that inherits from Piece and implements
///               queen-specific movement rules for the chess game.
/// @date         2026-02-06
/// @copyright    2026 Obsidian Honor Coders
/// @license      Apache License 2.0 - http://www.apache.org/licenses/LICENSE-2.0
/// @note         Licensed under the Apache License, Version 2.0 (the "License");
///               you may not use this file except in compliance with the License.

#ifndef ICHESS_SRC_QUEEN
#define ICHESS_SRC_QUEEN

#include "pieces.hpp"

/// @class   Queen
/// @brief   Represents a Queen chess piece.
/// @details Inherits from Piece and implements queen-specific movement rules.
///          Queens move horizontally, vertically, or diagonally any number of squares.
class Queen : public Piece
{
  public:
    /// @brief Construct a Queen at the specified position and color.
    /// @param file The file (column) coordinate ('a' to 'h').
    /// @param rank The rank (row) coordinate ('1' to '8').
    /// @param color The color of the queen (WHITE or BLACK).
    Queen(char file, char rank, PieceColor color) : Piece(color, PieceType::QUEEN) { set_position(file, rank); }

    /// @brief Virtual destructor for proper cleanup.
    virtual ~Queen() override {};

    /// @brief      Calculate valid moves for this queen.
    /// @param[out] p     Vector to be filled with valid move positions.
    /// @param[in]  other Vector of pointers to all other pieces on the board for move validation.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Queens combine rook and bishop movement patterns.
    virtual void moves(std::vector<PiecePosition>& p, const PieceList other) const override;

    /// @brief      Calculate valid moves for this queen.
    /// @param[out] p        Vector to be filled with valid move positions.
    /// @param[in]  other_p  Vector of positions of all other pieces on the board for move validation.
    /// @param[in]  other_c  Vector of colors corresponding to each piece in other_p for determining valid captures.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Queens combine rook and bishop movement patterns.
    /// @details    This overload provides piece positions and colors separately for move calculation.
    virtual void moves(std::vector<PiecePosition>&      p,
                       const std::vector<PiecePosition> other_p,
                       const std::vector<PieceColor>    other_c) const override;
};

#endif // ICHESS_SRC_QUEEN
