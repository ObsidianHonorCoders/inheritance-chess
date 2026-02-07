/// @file      bishop.cpp
/// @author    Calileus (https://github.com/Calileus/inheritance-chess)
/// @brief     Implementation of the Bishop chess piece class.
/// @details   Provides Bishop-specific functionality and move calculation.
///            Currently serves as a placeholder for future bishop movement logic.
/// @version   1.0
/// @date      2026-02-06
/// @copyright MIT License - see LICENSE file for details

#include <iostream>
#include "bishop.hpp"

void Bishop::moves(std::vector<PiecePosition>& p, const PieceList other) const {}

void Bishop::moves(std::vector<PiecePosition>&      p,
                   const std::vector<PiecePosition> other_p,
                   const std::vector<PieceColor>    other_c) const
{
}
