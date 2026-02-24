/// @file      rook.hpp
/// @brief     Rook piece implementation for the chess engine.
/// @author    Calileus
/// @date      2026-02-06
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines the Rook class that inherits from Piece and implements
///            rook-specific movement rules for the chess game.

#ifndef ICHESS_SRC_ROOK
#define ICHESS_SRC_ROOK

#include "pieces.hpp"

/// @class   Rook
/// @brief   Represents a Rook chess piece.
/// @details Inherits from Piece and implements rook-specific movement rules.
///          Rooks move horizontally or vertically any number of squares.
class Rook : public Piece
{
  public:
    /// @brief Construct a Rook at the specified position and color.
    /// @param file The file (column) coordinate ('a' to 'h').
    /// @param rank The rank (row) coordinate ('1' to '8').
    /// @param color The color of the rook (WHITE or BLACK).
    Rook(char file, char rank, Piece::Color color) : Piece(color, Piece::Type::ROOK) { set_position(file, rank); }

    /// @brief Virtual destructor for proper cleanup.
    virtual ~Rook() override {};

    /// @brief      Calculate valid moves for this rook.
    /// @param[out] p        Vector to be filled with valid move positions.
    /// @param[in]  other_p  Vector of positions of all other pieces on the board for move validation.
    /// @param[in]  other_c  Vector of colors corresponding to each piece in other_p for determining valid captures.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Rooks move horizontally and vertically without restriction.
    /// @details    This overload provides piece positions and colors separately for move calculation.
    virtual void available_moves(Piece::PositionList&       p,
                                 const Piece::PositionList& other_p,
                                 const Piece::ColorList&    other_c) const override;
};

#endif // ICHESS_SRC_ROOK
