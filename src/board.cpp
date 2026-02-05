/// ---------------------------------------------------------------------------------
/// @file      board.cpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Chess board console type and display.
/// @version   1.0
/// @date      2026-01-22
/// @copyright MIT License - see LICENSE file for details
/// ---------------------------------------------------------------------------------
#include "board.hpp"

Board::Board() { clearBoard(); }

void Board::clearBoard()
{
  for (int i = 0; i < Board::MAX_OUT_EACH_SIDE_BOARD; i++)
  {
    if (w_out[i])
    {
      delete w_out[i];
      w_out[i] = nullptr;
    }
    if (b_out[i])
    {
      delete b_out[i];
      b_out[i] = nullptr;
    }
  }
  for (int i = 0; i < Board::BOARD_SIZE; i++)
  {
    for (int j = 0; j < Board::BOARD_SIZE; j++)
    {
      if (grid[i][j])
      {
        delete grid[i][j];
        grid[i][j] = nullptr;
      }
    }
  }
}

void Board::initializeStandardSetup() {}

void Board::display() const
{
  for (Pieza* p : w_out)
  {
    std::cout << getchar(p);
  }
  for (int row = 0; row < BOARD_SIZE; row++)
  {
    std::cout << std::endl << "  +---+---+---+---+---+---+---+---+" << std::endl;
    std::cout << (8 - row) << " |";
    for (int col = 0; col < BOARD_SIZE; col++)
    {
      if ((row + col) % 2 == 0)
      {
        std::cout << " " << getchar(grid[row][col]) << " |";
      }
      else
      {
        std::cout << "-" << getchar(grid[row][col]) << "-|";
      }
    }
  }
  std::cout << std::endl << "  +---+---+---+---+---+---+---+---+" << std::endl;
  std::cout << "    a   b   c   d   e   f   g   h" << std::endl << std::endl;
  for (Pieza* p : b_out)
  {
    std::cout << getchar(p);
  }
}

void Board::addPieceOut(bool side, Pieza* piece)
{
  int idx = 0;
  if (side)
  {
    while (idx < Board::MAX_OUT_EACH_SIDE_BOARD && w_out[idx])
    {
      idx++;
    }
    if (idx < Board::MAX_OUT_EACH_SIDE_BOARD)
    {
      w_out[idx] = piece;
    }
    else
    {
      throw std::runtime_error("White out stack is full.");
    }
  }
  else
  {
    while (idx < Board::MAX_OUT_EACH_SIDE_BOARD && b_out[idx])
    {
      idx++;
    }
    if (idx < Board::MAX_OUT_EACH_SIDE_BOARD)
    {
      b_out[idx] = piece;
    }
    else
    {
      throw std::runtime_error("Black out stack is full.");
    }
  }
}

void Board::setPiece(int row, int col, Pieza* piece)
{
  if (0 <= row && row < BOARD_SIZE && 0 <= col && col < BOARD_SIZE)
  {
    if (grid[row][col])
    {
      addPieceOut(grid[row][col]->is_white(), grid[row][col]);
    }
    grid[row][col] = piece;
  }
}
