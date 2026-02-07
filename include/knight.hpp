/// @file      knight.hpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Knight piece implementation for the chess engine.
/// @details   Defines the Knight class that inherits from Piece and implements
///            knight-specific movement rules for the chess game.
/// @version   1.0
/// @date      2026-02-06
/// @copyright MIT License - see LICENSE file for details

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
    Knight(char file, char rank, PieceColor color) : Piece(color, PieceType::KNIGHT) { set_position(file, rank); }

    /// @brief Virtual destructor for proper cleanup.
    virtual ~Knight() override {};

    /// @brief      Calculate valid moves for this knight.
    /// @param[out] p     Vector to be filled with valid move positions.
    /// @param[in]  other Vector of pointers to all other pieces on the board for move validation.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Knights move in an L-shape pattern, regardless of other pieces on the board.
    virtual void moves(std::vector<PiecePosition>& p, const PieceList other) const override;

    /// @brief      Calculate valid moves for this knight.
    /// @param[out] p        Vector to be filled with valid move positions.
    /// @param[in]  other_p  Vector of positions of all other pieces on the board for move validation.
    /// @param[in]  other_c  Vector of colors corresponding to each piece in other_p for determining valid captures.
    /// @throws     std::runtime_error if the piece has an invalid color.
    /// @note       Knights move in an L-shape pattern, regardless of other pieces on the board.
    /// @details    This overload provides piece positions and colors separately for move calculation.
    virtual void moves(std::vector<PiecePosition>&      p,
                       const std::vector<PiecePosition> other_p,
                       const std::vector<PieceColor>    other_c) const override;
};

#endif // ICHESS_SRC_KNIGHT
