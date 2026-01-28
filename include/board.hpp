/// ---------------------------------------------------------------------------------
/// @file      board.hpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Chess board console representation and display.
/// @version   1.0
/// @date      2026-01-22
/// @copyright MIT License - see LICENSE file for details
/// ---------------------------------------------------------------------------------
#ifndef ICHESS_SRC_BOARD
#define ICHESS_SRC_BOARD

#include <array>
#include <iostream>

class Board
{
  public:
    enum class PieceCharID : char
    {
      EMPTY        = ' ',
      PAWN_WHITE   = 'P',
      PAWN_BLACK   = 'p',
      ROOK_WHITE   = 'R',
      ROOK_BLACK   = 'r',
      KNIGHT_WHITE = 'N',
      KNIGHT_BLACK = 'n',
      BISHOP_WHITE = 'B',
      BISHOP_BLACK = 'b',
      QUEEN_WHITE  = 'Q',
      QUEEN_BLACK  = 'q',
      KING_WHITE   = 'K',
      KING_BLACK   = 'k'
    };

    static constexpr int BOARD_SIZE = 8;

    Board();
    void        initializeStandardSetup();
    void        display() const;
    void        setPiece(int row, int col, PieceCharID piece);
    PieceCharID getPiece(int row, int col) const;

  private:
    std::array<std::array<PieceCharID, BOARD_SIZE>, BOARD_SIZE> grid;
};

#endif // ICHESS_SRC_BOARD
