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
#include <stdexcept>

#include "common.hpp"

class Pieza
{
  private:
    PieceColor    color;
    PiecePosition position;
    PieceType     type;

    Pieza(); // Private constructor to prevent color-less instantiation

  public:
    Pieza(PieceColor col, PieceType typ);

    const bool is_black() const;
    const bool is_white() const;
    void       set_position(const char f, const char r);
    void       get_position(char& f, char& r);
    const char get_representation() const;

    virtual ~Pieza()                                        = default;
    virtual void moves(std::vector<PiecePosition>& p) const = 0;
};

inline const char getchar(Pieza* p) { return (p ? p->get_representation() : ' '); }

#endif // ICHESS_SRC_PIEZAS
