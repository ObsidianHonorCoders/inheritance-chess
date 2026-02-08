/// @file      pawns.hpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Pawn piece implementation for the chess engine.
/// @details   Defines the Pawn class that inherits from Piece and implements
///            pawn-specific movement rules for the chess game.
/// @version   1.0
/// @date      2026-01-01
/// @copyright MIT License - see LICENSE file for details

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
    Pawn(char file, char rank, PieceColor color) : Piece(color, PieceType::PAWN) { set_position(file, rank); }

    /// @brief Virtual destructor for proper cleanup.
    virtual ~Pawn() override {};

    /// @brief      Calculate valid moves for this pawn.
    /// @param[out] p     Vector to be filled with valid move positions.
    /// @param[in]  other Vector of pointers to all other pieces on the board for move validation.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Implementation distinguishes between white and black pawns for directional movement.
    virtual void moves(std::vector<PiecePosition>& p, const PieceList other) const override;

    /// @brief      Calculate valid moves for this pawn.
    /// @param[out] p        Vector to be filled with valid move positions.
    /// @param[in]  other_p  Vector of positions of all other pieces on the board for move validation.
    /// @param[in]  other_c  Vector of colors corresponding to each piece in oth_p for determining valid captures.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Implementation distinguishes between white and black pawns for directional movement.
    /// @details    This overload provides piece positions and colors separately for move calculation.
    virtual void moves(std::vector<PiecePosition>&      p,
                       const std::vector<PiecePosition> other_p,
                       const std::vector<PieceColor>    other_c) const override;
};

#endif // ICHESS_SRC_PAWNS
