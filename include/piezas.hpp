/// ---------------------------------------------------------------------------------
/// @file      piezas.hpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Basic Chess Engine demonstrating C++ inheritance and polymorphism.
/// @version   1.0
/// @date      2026-01-01
/// @copyright MIT License - see LICENSE file for details
/// ---------------------------------------------------------------------------------
#ifndef ICHESS_SRC_PIEZAS
#define ICHESS_SRC_PIEZAS
#include <vector>
#include <cctype>
#include "common.hpp"

class Pieza
{
  private:
    PieceColor          color;
    PiecePosition       position;
    PieceRepresentation representation;

    Pieza() {}; // Private constructor to prevent color-less instantiation

  public:
    Pieza(PieceColor col) : color(col) { set_position('a', '1'); }

    bool        is_black() const { return color == PieceColor::BLACK; }
    bool        is_white() const { return color == PieceColor::WHITE; }
    inline void set_position(char f, char r)
    {
      position.file = f;
      position.rank = r;
    }
    inline char get_representation() const
    {
      char res;
      switch (color)
      {
      case PieceColor::WHITE:
        return static_cast<char>(representation);
      case PieceColor::BLACK:
        return static_cast<char>(std::tolower(static_cast<char>(representation)));
      default:
        return ' ';
      }
    }

    virtual ~Pieza()                                        = default;
    virtual void moves(std::vector<PiecePosition>& p) const = 0;
};

#endif // ICHESS_SRC_PIEZAS
