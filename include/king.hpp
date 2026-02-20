/// @file      king.hpp
/// @brief     King piece implementation for the chess engine.
/// @author    Calileus
/// @date      2026-02-06
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines the King class that inherits from Piece and implements
///            king-specific movement rules for the chess game.

#ifndef ICHESS_SRC_KING
#define ICHESS_SRC_KING

#include "pieces.hpp"

/// @class   King
/// @brief   Represents a King chess piece.
/// @details Inherits from Piece and implements king-specific movement rules.
///          Kings move one square in any direction (horizontal, vertical, or diagonal).
class King : public Piece
{
  public:
    /// @brief Construct a King at the specified position and color.
    /// @param file The file (column) coordinate ('a' to 'h').
    /// @param rank The rank (row) coordinate ('1' to '8').
    /// @param color The color of the king (WHITE or BLACK).
    King(char file, char rank, Piece::Color color) : Piece(color, Piece::Type::KING) { set_position(file, rank); }

    /// @brief Virtual destructor for proper cleanup.
    virtual ~King() override {};

    /// @brief      Calculate valid moves for this king.
    /// @param[out] p     Vector to be filled with valid move positions.
    /// @param[in]  other Vector of unique pointers to all other pieces on the board for move validation.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Kings move one square in any direction without restriction.
    virtual void moves(Piece::PositionList& p, const Piece::List& other) const override;

    /// @brief      Calculate valid moves for this king.
    /// @param[out] p        Vector to be filled with valid move positions.
    /// @param[in]  other_p  Vector of positions of all other pieces on the board for move validation.
    /// @param[in]  other_c  Vector of colors corresponding to each piece in other_p for determining valid captures.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Kings move one square in any direction without restriction.
    /// @details    This overload provides piece positions and colors separately for move calculation.
    virtual void moves(Piece::PositionList& p, const Piece::PositionList& other_p, const Piece::ColorList& other_c) const override;
};

#endif // ICHESS_SRC_KING
