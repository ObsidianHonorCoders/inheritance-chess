/// @file      board.cpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Implementation of the Board class for chess game state management.
/// @details   Provides implementation for piece management, grid updates, and
///            console-based board visualization using ASCII art.
/// @version   1.0
/// @date      2026-01-22
/// @copyright MIT License - see LICENSE file for details

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
  for (Piece* p : pieces)
  {
    if (p)
    {
      p->get_position(f, r);
      grid[f - 'a']['8' - r] = p->get_representation();
    }
  }
}

/// @brief   Remove and delete all pieces from the board.
/// @details Iterates through the pieces container in reverse order, deletes each piece,
///          and removes it from the vector to prevent memory leaks.
void Board::cleanPieces()
{
  for (size_t i = pieces.size(); i > 0; i--)
  {
    if (pieces[i - 1])
    {
      delete pieces[i - 1];
      pieces[i - 1] = NULL;
    }
    pieces.pop_back();
  }
}

/// @brief   Add a piece to the board.
/// @param   piece Pointer to the Piece object to add. Deletion is managed by Borad class.
/// @details The board takes ownership of the piece and will manage its lifetime.
void Board::addPiece(Piece* piece) { pieces.push_back(piece); }

/// @brief   Initialize the board with standard chess starting position.
/// @details Creates white pices on rank 1 and 2 and black pieces on rank 7 and 8.
///          Updates the grid representation after placing pieces.
void Board::initializeStandardSetup()
{
  clearGrid();
  for (int i = 0; i < BOARD_SIZE; i++)
  {
    addPiece(new Pawn('a' + i, '2', PieceColor::WHITE));
    addPiece(new Pawn('a' + i, '7', PieceColor::BLACK));
  }
  addPiece(new Rook('a', '1', PieceColor::WHITE));
  addPiece(new Rook('h', '1', PieceColor::WHITE));
  addPiece(new Rook('a', '8', PieceColor::BLACK));
  addPiece(new Rook('h', '8', PieceColor::BLACK));
  addPiece(new Knight('b', '1', PieceColor::WHITE));
  addPiece(new Knight('g', '1', PieceColor::WHITE));
  addPiece(new Knight('b', '8', PieceColor::BLACK));
  addPiece(new Knight('g', '8', PieceColor::BLACK));
  addPiece(new Bishop('c', '1', PieceColor::WHITE));
  addPiece(new Bishop('f', '1', PieceColor::WHITE));
  addPiece(new Bishop('c', '8', PieceColor::BLACK));
  addPiece(new Bishop('f', '8', PieceColor::BLACK));
  addPiece(new Queen('d', '1', PieceColor::WHITE));
  addPiece(new Queen('d', '8', PieceColor::BLACK));
  addPiece(new King('e', '1', PieceColor::WHITE));
  addPiece(new King('e', '8', PieceColor::BLACK));
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
