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
    ~Board();
    void clearGrid();
    void updateGrid();
    void cleanPieces();
    void addPiece(Pieza* piece);
    void initializeStandardSetup();
    void display() const;

  private:
    std::vector<Pieza*>                                  pieces = {};
    std::array<std::array<char, BOARD_SIZE>, BOARD_SIZE> grid   = {};
};

#endif // ICHESS_SRC_BOARD
