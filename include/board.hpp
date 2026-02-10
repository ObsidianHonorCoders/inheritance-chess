/// @file         board.hpp
/// @brief        Chess board management and console display.
/// @project      Inheritance Chess
/// @organization Obsidian Honor Coders
/// @repository   https://github.com/ObsidianHonorCoders/inheritance-chess
/// @author       Calileus
/// @details      Defines the Board class that manages chess pieces, board state,
///               and provides console-based visualization.
/// @date         2026-01-22
/// @copyright    2026 Obsidian Honor Coders
/// @license      Apache License 2.0 - http://www.apache.org/licenses/LICENSE-2.0
/// @note         Licensed under the Apache License, Version 2.0 (the "License");
///               you may not use this file except in compliance with the License.

#ifndef ICHESS_SRC_BOARD
#define ICHESS_SRC_BOARD

#include <array>
#include <iostream>

#include "common.hpp"
#include "pieces.hpp"

inline constexpr int BOARD_SIZE              = 8;  ///< Standard chess board size (8x8)
inline constexpr int MAX_OUT_EACH_SIDE_BOARD = 15; ///< Maximum display padding on each side

/// @brief A matix of characters representing chess pieces.
/// @note  This container has fixed size. It store char that represent how pieces are
///        goig to be displayed on the console. Space ' ' shall be used for empty square.
using BoardGrid = std::array<std::array<char, BOARD_SIZE>, BOARD_SIZE>;

/// @class   Board
/// @brief   Manages the chess board state and piece placement.
/// @details The Board class handles piece management, maintains a grid representation
///          of the board state, and provides console-based display functionality.
class Board
{
  public:
    /// @brief   Construct a new Board.
    /// @details Initializes an empty board with cleared grid.
    Board();

    /// @brief   Destruct the Board.
    /// @details Cleans up all pieces and frees allocated memory.
    ~Board();

    /// @brief   Clear all pieces from the grid.
    /// @details Sets all grid positions to space character ' '.
    void clearGrid();

    /// @brief   Update the grid representation from current piece positions.
    /// @details Synchronizes the visual grid with the actual piece positions.
    void updateGrid();

    /// @brief   Delete all pieces and clear the pieces container.
    /// @details Frees memory for all dynamically allocated pieces.
    void cleanPieces();

    /// @brief Add a piece to the board.
    /// @note  The board takes ownership of the piece pointer.
    void addPiece(Piece* piece);

    /// @brief   Initialize the board with standard chess starting position.
    /// @details Sets up all pawns in their starting positions for a new game.
    void initializeStandardSetup();

    /// @brief   Display the current board state to console.
    /// @details Outputs an ASCII representation of the board with piece positions,
    ///          file labels (a-h) and rank numbers (1-8).
    void display() const;

  private:
    PieceList pieces = {}; ///< Collection of pieces currently on the board
    BoardGrid grid   = {}; ///< 8x8 character grid for display
};

#endif // ICHESS_SRC_BOARD
