/// ---------------------------------------------------------------------------------
/// @file      board.hpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Chess board console type and display.
/// @version   1.0
/// @date      2026-01-22
/// @copyright MIT License - see LICENSE file for details
/// ---------------------------------------------------------------------------------
#ifndef ICHESS_SRC_BOARD
#define ICHESS_SRC_BOARD

#include <array>
#include <iostream>

#include "common.hpp"
#include "pawns.hpp"

class Board
{
  public:
    static constexpr int BOARD_SIZE              = 8;
    static constexpr int MAX_OUT_EACH_SIDE_BOARD = 15;

    Board();
    void clearBoard();
    void initializeStandardSetup();
    void display() const;
    void addPieceOut(bool side, Pieza* piece);
    void setPiece(int row, int col, Pieza* piece);

  private:
    std::array<std::array<Pieza*, BOARD_SIZE>, BOARD_SIZE> grid  = {};
    std::array<Pieza*, MAX_OUT_EACH_SIDE_BOARD>            w_out = {}, b_out = {};
};

#endif // ICHESS_SRC_BOARD
