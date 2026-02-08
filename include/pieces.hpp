/// @file      pieces.hpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Base class for all chess pieces using polymorphism.
/// @details   Defines the abstract Piece (piece) class that serves as the base
///            for all specific piece types in the chess engine.
/// @version   1.0
/// @date      2026-01-01
/// @copyright MIT License - see LICENSE file for details

#ifndef ICHESS_SRC_PIEZAS
#define ICHESS_SRC_PIEZAS

#include <vector>
#include <stdexcept>

#include "common.hpp"

/// @class   Piece
/// @brief   Base abstract class representing a chess piece.
/// @details This class serves as the polymorphic base for all chess pieces (pawns, knights, etc.)
///          It manages piece color, position, and type. Derived classes must implement
///          the moves() method to define piece-specific movement rules.
class Piece
{
  private:
    PieceColor    color;    ///< Color of the piece (white, black, or none)
    PiecePosition position; ///< Current position of the piece on the board
    PieceType     type;     ///< Type of the piece (pawn, knight, bishop, etc.)

    /// @brief   Private default constructor.
    /// @details Prevents instantiation of Piece without a color and type.
    Piece();

  public:
    /// @brief Construct a Piece with specified color and type.
    /// @param col The color of the piece (WHITE or BLACK).
    /// @param typ The type of the piece (PAWN, KNIGHT, BISHOP, ROOK, QUEEN, or KING).
    Piece(PieceColor col, PieceType typ);

    /// @brief  Checks if the piece is black.
    /// @return True if the piece color is BLACK, false otherwise.
    const bool is_black() const;

    /// @brief  Checks if the piece is white.
    /// @return True if the piece color is WHITE, false otherwise.
    const bool is_white() const;

    /// @brief   Set the position of the piece on the board.
    /// @param   f The file (column) character ('a' to 'h').
    /// @param   r The rank (row) character ('1' to '8').
    /// @details Invalid coordinates will be set to space character ' '.
    void set_position(const char f, const char r);

    /// @brief      Get the current position of the piece (file: 'a'-'h', rank: '1'-'8').
    /// @param[out] f Reference to store the file (column) coordinate.
    /// @param[out] r Reference to store the rank (row) coordinate.
    void get_position(char& f, char& r);

    /// @brief  Get the character representation of the piece.
    /// @return Character representation (uppercase for white, lowercase for black).
    const char get_representation() const;

    /// @brief Virtual destructor for proper cleanup in derived classes.
    virtual ~Piece() = default;

    /// @brief      Pure virtual method to calculate valid moves for the piece.
    /// @param[out] p     Vector to be filled with valid move positions.
    /// @param[in]  other Vector of pointers to all other pieces on the board for move validation.
    /// @note       Must be implemented by derived classes for their specific movement rules.
    virtual void moves(std::vector<PiecePosition>& p, const std::vector<Piece*> other) const = 0;

    /// @brief      Pure virtual method to calculate valid moves for the piece.
    /// @param[out] p       Vector to be filled with valid move positions.
    /// @param[in]  other_p Vector of positions of all other pieces on the board for move validation.
    /// @param[in]  other_c Vector of colors corresponding to each piece in oth_p for determining valid captures.
    /// @note       Must be implemented by derived classes for their specific movement rules.
    /// @details    This overload provides piece positions and colors separately, allowing independent
    ///             validation of piece positions and ownership for move calculation.
    virtual void
    moves(std::vector<PiecePosition>& p, const std::vector<PiecePosition> other_p, const std::vector<PieceColor> other_c) const = 0;
};

/// @brief A list of pointers to chess pieces.
/// @note  This container does NOT own the memory. Objects must be manually deleted
///        or managed by the Board class to avoid memory leaks.
using PieceList = std::vector<Piece*>;

/// @brief  Helper function to safely get the character representation of a piece pointer.
/// @param  p Pointer to a Piece object (may be null).
/// @return Character representation of the piece, or space ' ' if pointer is null.
inline const char getchar(Piece* p) { return (p ? p->get_representation() : ' '); }

#endif // ICHESS_SRC_PIEZAS
