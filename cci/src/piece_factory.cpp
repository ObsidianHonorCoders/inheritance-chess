/// @file      piece_factory.cpp
/// @namespace Chess
/// @brief     Chess Common Interface (CCI) - Piece Factory Implementation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implementation of piece factory and smart pointer patterns
///            adapted from existing Piece creation patterns.

#include "grid.h"
#include "../cpl/include/chess_pawn.h"
#include "../cpl/include/chess_knight.h"
#include "../cpl/include/chess_bishop.h"
#include "../cpl/include/chess_rook.h"
#include "../cpl/include/chess_queen.h"
#include "../cpl/include/chess_king.h"
#include <stdexcept>

namespace Chess
{

    /// @brief Implementation of ChessPiece::available_moves using PieceList parameter.
    /// @details Extracts positions and colors from PieceList and calls pure virtual overload.
    ///          Adapted from existing Piece::available_moves pattern.
    void ChessPiece::available_moves(PositionList& moves, const PieceList& other, const Grid& grid) const
    {
        // Extract positions and colors from PieceList (adapted from existing pattern)
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

    /// @brief Factory function implementation for creating chess pieces.
    /// @details Factory pattern adapted from existing piece creation patterns.
    ///          Uses smart pointers for automatic memory management.
    std::unique_ptr<ChessPiece> create_piece(PieceType type, Color color, const Position& pos)
    {
        // Phase 2 implementation - create concrete piece types
        // This follows the existing pattern from piece creation in the original codebase
        // where pieces were created with specific constructors like:
        // auto pawn = std::make_unique<Pawn>('e', '2', Piece::Color::WHITE);
        
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
