/// @file      board.cpp
/// @brief     Implementation of the Board class for chess game state management.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @details   Provides implementation for piece management, grid updates, and
///            console-based board visualization using ASCII art.

#include "board.hpp"
#include "pawns.hpp"
#include "rook.hpp"
#include "knight.hpp"
#include "bishop.hpp"
#include "queen.hpp"
#include "king.hpp"

/// @brief   Construct the Board.
/// @details Initializes a new board with an empty grid.
Board::Board() { clearGrid(); }

/// @brief   Destruct the Board.
/// @details Cleans up all pieces and frees allocated memory.
Board::~Board() { cleanPieces(); }

/// @brief   Clear the board grid.
/// @details Sets all grid cells to space characters, effectively clearing the board display.
void Board::clearGrid()
{
  for (int i = 0; i < BOARD_SIZE; i++)
  {
    for (int j = 0; j < BOARD_SIZE; j++)
    {
      grid[i][j] = ' ';
    }
  }
}

/// @brief   Update the grid representation from piece positions.
/// @details Iterates through all pieces and updates their positions in the grid.
///          Converts file/rank coordinates to grid array indices.
void Board::updateGrid()
{
  char f = ' ', r = ' ';
  for (const std::unique_ptr<Piece>& p : pieces)
  {
    if (p)
    {
      p->get_position(f, r);
      grid[f - 'a']['8' - r] = p->get_representation();
    }
  }
}

/// @brief   Remove and delete all pieces from the board.
/// @details Smart pointers automatically clean up memory when vector is cleared.
void Board::cleanPieces()
{
  pieces.clear();
}

/// @brief   Add a piece to the board.
/// @param   piece Unique pointer to the Piece object to add. Ownership is transferred to Board class.
/// @details The board takes ownership of the piece and will manage its lifetime.
void Board::addPiece(std::unique_ptr<Piece> piece) { pieces.push_back(std::move(piece)); }

/// @brief   Initialize the board with standard chess starting position.
/// @details Creates white pieces on rank 1 and 2 and black pieces on rank 7 and 8.
///          Updates the grid representation after placing pieces.
void Board::initializeStandardSetup()
{
  clearGrid();
  for (int i = 0; i < BOARD_SIZE; i++)
  {
    addPiece(std::make_unique<Pawn>('a' + i, '2', Piece::Color::WHITE));
    addPiece(std::make_unique<Pawn>('a' + i, '7', Piece::Color::BLACK));
  }
  addPiece(std::make_unique<Rook>('a', '1', Piece::Color::WHITE));
  addPiece(std::make_unique<Rook>('h', '1', Piece::Color::WHITE));
  addPiece(std::make_unique<Rook>('a', '8', Piece::Color::BLACK));
  addPiece(std::make_unique<Rook>('h', '8', Piece::Color::BLACK));
  addPiece(std::make_unique<Knight>('b', '1', Piece::Color::WHITE));
  addPiece(std::make_unique<Knight>('g', '1', Piece::Color::WHITE));
  addPiece(std::make_unique<Knight>('b', '8', Piece::Color::BLACK));
  addPiece(std::make_unique<Knight>('g', '8', Piece::Color::BLACK));
  addPiece(std::make_unique<Bishop>('c', '1', Piece::Color::WHITE));
  addPiece(std::make_unique<Bishop>('f', '1', Piece::Color::WHITE));
  addPiece(std::make_unique<Bishop>('c', '8', Piece::Color::BLACK));
  addPiece(std::make_unique<Bishop>('f', '8', Piece::Color::BLACK));
  addPiece(std::make_unique<Queen>('d', '1', Piece::Color::WHITE));
  addPiece(std::make_unique<Queen>('d', '8', Piece::Color::BLACK));
  addPiece(std::make_unique<King>('e', '1', Piece::Color::WHITE));
  addPiece(std::make_unique<King>('e', '8', Piece::Color::BLACK));
  updateGrid();
}

/// @brief   Display the current board state to console.
/// @details Outputs an ASCII representation of the chess board with:
///          - Piece positions from the internal grid
///          - Alternating square shading using '-' and space characters
///          - File labels (a-h) along the bottom
///          - Rank numbers (1-8) along the left side
void Board::display() const
{
  for (int row = 0; row < BOARD_SIZE; row++)
  {
    std::cout << std::endl << "  +---+---+---+---+---+---+---+---+" << std::endl;
    std::cout << (8 - row) << " |";
    for (int col = 0; col < BOARD_SIZE; col++)
    {
      if ((row + col) % 2 == 0)
      {
        std::cout << " " << grid[col][row] << " |";
      }
      else
      {
        std::cout << "-" << grid[col][row] << "-|";
      }
    }
  }
  std::cout << std::endl << "  +---+---+---+---+---+---+---+---+" << std::endl;
  std::cout << "    a   b   c   d   e   f   g   h" << std::endl << std::endl;
}
