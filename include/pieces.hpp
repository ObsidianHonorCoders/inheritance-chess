/// @file         pieces.hpp
/// @brief        Base class for all chess pieces using polymorphism.
/// @project      Inheritance Chess
/// @organization Obsidian Honor Coders
/// @repository   https://github.com/ObsidianHonorCoders/inheritance-chess
/// @author       Calileus
/// @details      Defines the abstract Piece (piece) class that serves as the base
///               for all specific piece types in the chess engine.
/// @date         2026-01-01
/// @copyright    2026 Obsidian Honor Coders
/// @license      Apache License 2.0 - http://www.apache.org/licenses/LICENSE-2.0
/// @note         Licensed under the Apache License, Version 2.0 (the "License");
///               you may not use this file except in compliance with the License.

#ifndef ICHESS_SRC_PIEZAS
#define ICHESS_SRC_PIEZAS

#include <vector>
#include <memory>
#include <stdexcept>

#include "common.hpp"

/// @class   Piece
/// @brief   Base abstract class representing a chess piece.
/// @details This class serves as the polymorphic base for all chess pieces (pawns, knights, etc.)
///          It manages piece color, position, and type. Derived classes must implement
///          the moves() method to define piece-specific movement rules.
class Piece
{
  public:
    /// @enum    Piece::Color
    /// @brief   Represents the color of a chess piece.
    /// @details Each chess piece belongs to either the white or black side, or no side (NONE).
    enum class Color : char
    {
      NONE  = ' ', ///< No color assigned
      WHITE = 'w', ///< White piece
      BLACK = 'b'  ///< Black piece
    };

    /// @enum    Piece::Type
    /// @brief   Represents the type of chess piece.
    /// @details Enumerates all standard chess piece types, each with a unique character representation.
    enum class Type : char
    {
      NONE   = ' ', ///< No piece type
      PAWN   = 'P', ///< Pawn piece
      KNIGHT = 'N', ///< Knight piece
      BISHOP = 'B', ///< Bishop piece
      ROOK   = 'R', ///< Rook piece
      QUEEN  = 'Q', ///< Queen piece
      KING   = 'K'  ///< King piece
    };

    /// @struct  Piece::Position
    /// @brief   Represents a position on the chess board.
    /// @details Uses algebraic notation where file (column) ranges from 'a' to 'h'
    ///          and rank (row) ranges from '1' to '8'.
    struct Position
    {
        char file; ///< File (column) coordinate, 'a' through 'h'
        char rank; ///< Rank (row) coordinate, '1' through '8'
    };

    /// @class Piece::List
    /// @brief A vector of unique pointers to chess pieces.
    /// @note  This container owns the memory. Objects are automatically managed
    ///        by smart pointers to prevent memory leaks.
    using List = std::vector<std::unique_ptr<Piece>>;

    /// @class Piece::PositionList
    /// @brief A vector of chess pieces positions.
    using PositionList = std::vector<Piece::Position>;

    /// @class Piece::ColorList
    /// @brief A vector of chess pieces colors.
    using ColorList = std::vector<Piece::Color>;

  private:
    Piece::Color    color;    ///< Color of the piece (white, black, or none)
    Piece::Type     type;     ///< Type of the piece (pawn, knight, bishop, etc.)
    Piece::Position position; ///< Current position of the piece on the board

    /// @brief   Private default constructor.
    /// @details Prevents instantiation of Piece without a color and type.
    Piece();

  public:
    /// @brief Construct a Piece with specified color and type.
    /// @param col The color of the piece (WHITE or BLACK).
    /// @param typ The type of the piece (PAWN, KNIGHT, BISHOP, ROOK, QUEEN, or KING).
    Piece(Piece::Color col, Piece::Type typ);

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

    /// @brief      Calculate valid moves for this piece using Piece::List parameter.
    /// @param[out] p     Vector to be filled with valid move positions.
    /// @param[in]  other Vector of unique pointers to all other pieces on the board for move validation.
    /// @details    This method extracts positions and colors from the List and calls the pure virtual overload.
    ///             Provides a convenient interface for callers using the Piece::List container.
    void moves(PositionList& p, const List& other) const;

    /// @brief      Pure virtual method to calculate valid moves for the piece.
    /// @param[out] p       Vector to be filled with valid move positions.
    /// @param[in]  other_p Vector of positions of all other pieces on the board for move validation.
    /// @param[in]  other_c Vector of colors corresponding to each piece in other_p for determining valid captures.
    /// @note       Must be implemented by derived classes for their specific movement rules.
    /// @details    This is the primary method that derived classes must implement.
    ///             It provides piece positions and colors separately for efficient move calculation.
    virtual void moves(PositionList& p, const PositionList& other_p, const ColorList& other_c) const = 0;
};

/// @brief  Helper function to safely get the character representation of a piece pointer.
/// @param  p Raw pointer to a Piece object (may be null).
/// @return Character representation of the piece, or space ' ' if pointer is null.
inline const char getchar(const Piece* p) { return (p ? p->get_representation() : ' '); }

#endif // ICHESS_SRC_PIEZAS
