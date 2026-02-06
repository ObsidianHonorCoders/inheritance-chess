/// ---------------------------------------------------------------------------------
/// @file      board.cpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Chess board console type and display.
/// @version   1.0
/// @date      2026-01-22
/// @copyright MIT License - see LICENSE file for details
/// ---------------------------------------------------------------------------------
#include "board.hpp"

Board::Board() { clearGrid(); }
Board::~Board() { cleanPieces(); }

void Board::clearGrid()
{
  for (int i = 0; i < Board::BOARD_SIZE; i++)
  {
    for (int j = 0; j < Board::BOARD_SIZE; j++)
    {
      grid[i][j] = ' ';
    }
  }
}

void Board::updateGrid()
{
  char f = ' ', r = ' ';
  for (Pieza* p : pieces)
  {
    if (p)
    {
      p->get_position(f, r);
      grid[f - 'a']['8' - r] = p->get_representation();
    }
  }
}

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

void Board::addPiece(Pieza* piece) { pieces.push_back(piece); }

void Board::initializeStandardSetup()
{
  clearGrid();
  for (int i = 0; i < BOARD_SIZE; i++)
  {
    addPiece(new Pawn('a' + i, '2', PieceColor::WHITE));
    addPiece(new Pawn('a' + i, '7', PieceColor::BLACK));
  }
  updateGrid();
}

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
