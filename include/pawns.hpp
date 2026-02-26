/// @file      pawns.hpp
/// @brief     Pawn piece implementation for the chess engine.
/// @author    Calileus
/// @date      2026-01-01
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines the Pawn class that inherits from Piece and implements
///            pawn-specific movement rules for the chess game.

#ifndef ICHESS_SRC_PAWNS
#define ICHESS_SRC_PAWNS

#include "pieces.hpp"

/// @class   Pawn
/// @brief   Represents a Pawn chess piece.
/// @details Inherits from Piece and implements pawn-specific movement rules.
///          Pawns move forward one square (or two squares from their starting position)
///          and capture diagonally.
class Pawn : public Piece
{
  public:
    /// @brief Construct a Pawn at the specified position and color.
    /// @param file The file (column) coordinate ('a' to 'h').
    /// @param rank The rank (row) coordinate ('1' to '8').
    /// @param color The color of the pawn (WHITE or BLACK).
    Pawn(char file, char rank, Piece::Color color) : Piece(color, Piece::Type::PAWN) { set_position(file, rank); }

    /// @brief Virtual destructor for proper cleanup.
    virtual ~Pawn() override {};

    /// @brief      Calculate valid moves for this pawn.
    /// @param[out] p        Vector to be filled with valid move positions.
    /// @param[in]  other_p  Vector of positions of all other pieces on the board for move validation.
    /// @param[in]  other_c  Vector of colors corresponding to each piece in other_p for determining valid captures.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Implementation distinguishes between white and black pawns for directional movement.
    /// @details    This overload provides piece positions and colors separately for move calculation.
    virtual void available_moves(Piece::PositionList&       p,
                                 const Piece::PositionList& other_p,
                                 const Piece::ColorList&    other_c) const override;
};

#endif // ICHESS_SRC_PAWNS
