/// @file      piece_factory.cpp
/// @namespace Chess
/// @brief     Chess Common Interface (CCI) - Piece Factory Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implementation of the create_piece() factory function and the
///            PieceList overload of ChessPiece::available_moves().

#include "grid.h"
#include "chess_pawn.h"
#include "chess_knight.h"
#include "chess_bishop.h"
#include "chess_rook.h"
#include "chess_queen.h"
#include "chess_king.h"
#include <stdexcept>

namespace Chess
{

    /// @brief Convert PieceList input to position/color lists and delegate move generation.
    /// @details Extracts positions and colors from PieceList into parallel vectors
    ///          then delegates to the pure virtual (PositionList, ColorList) overload.
    void ChessPiece::available_moves(PositionList& moves, const PieceList& other, const Grid& grid) const
    {
        // Extract positions and colors from PieceList.
        PositionList other_positions;
        ColorList other_colors;
        
        other_positions.reserve(other.size());
        other_colors.reserve(other.size());
        
        for (const auto& piece : other)
        {
            if (piece)
            {
                other_positions.push_back(piece->get_position());
                other_colors.push_back(piece->get_color());
            }
        }
        
        // Call the pure virtual overload
        available_moves(moves, other_positions, other_colors, grid);
    }

    /// @brief Factory function that constructs the concrete piece subclass for
    ///        the given type, color, and position and returns it as unique_ptr<ChessPiece>.
    std::unique_ptr<ChessPiece> create_piece(PieceType type, Color color, const Position& pos)
    {
        switch (type)
        {
            case PieceType::PAWN:
                return std::make_unique<ChessPawn>(color, pos);
            case PieceType::KNIGHT:
                return std::make_unique<ChessKnight>(color, pos);
            case PieceType::BISHOP:
                return std::make_unique<ChessBishop>(color, pos);
            case PieceType::ROOK:
                return std::make_unique<ChessRook>(color, pos);
            case PieceType::QUEEN:
                return std::make_unique<ChessQueen>(color, pos);
            case PieceType::KING:
                return std::make_unique<ChessKing>(color, pos);
            default:
                throw std::runtime_error("Invalid piece type");
        }
    }

} // namespace Chess


