/// @file      pieces_logic.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Pieces Logic (CPL).
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements piece move generation and attack detection.

#include "pieces_logic.h"

namespace Chess
{

  bool ChessPiecesLogic::is_raw_logical_move(const Grid& grid, const Move& move) const
  {
    auto moves = list_raw_logical_moves(grid, move.start_pos);
    for (const auto& m : moves)
    {
      if (m.end_pos == move.end_pos && m.flags == move.flags)
      {
        return true;
      }
    }
    return false;
  }

  std::vector<Move> ChessPiecesLogic::list_raw_logical_moves(const Grid& grid, Position position) const
  {
    if (!grid.is_occupied(position))
    {
      return {};
    }

    const auto& piece = grid.get_piece(position);
    if (!piece.has_value())
    {
      return {};
    }

    switch (piece->type)
    {
    case PieceType::PAWN:
      return get_pawn_moves(grid, position);
    case PieceType::KNIGHT:
      return get_knight_moves(grid, position);
    case PieceType::BISHOP:
      return get_bishop_moves(grid, position);
    case PieceType::ROOK:
      return get_rook_moves(grid, position);
    case PieceType::QUEEN:
      return get_queen_moves(grid, position);
    case PieceType::KING:
      return get_king_moves(grid, position);
    default:
      return {};
    }
  }

  bool ChessPiecesLogic::is_square_attacked(const Grid& grid, Position square, Color by_color) const
  {
    // Check all pieces of the attacking color
    for (int file = 0; file < 8; file++)
    {
      for (int rank = 0; rank < 8; rank++)
      {
        Position pos(file, rank);
        if (grid.is_occupied(pos))
        {
          const auto& piece = grid.get_piece(pos);
          if (piece.has_value() && piece->color == by_color)
          {
            auto moves = list_raw_logical_moves(grid, pos);
            for (const auto& move : moves)
            {
              if (move.end_pos == square)
              {
                return true;
              }
            }
          }
        }
      }
    }
    return false;
  }

  std::vector<Move> ChessPiecesLogic::get_pawn_moves(const Grid& grid, Position pos) const
  {
    // TODO: Implement pawn move logic
    return {};
  }

  std::vector<Move> ChessPiecesLogic::get_knight_moves(const Grid& grid, Position pos) const
  {
    // TODO: Implement knight move logic
    return {};
  }

  std::vector<Move> ChessPiecesLogic::get_bishop_moves(const Grid& grid, Position pos) const
  {
    // TODO: Implement bishop move logic
    return {};
  }

  std::vector<Move> ChessPiecesLogic::get_rook_moves(const Grid& grid, Position pos) const
  {
    // TODO: Implement rook move logic
    return {};
  }

  std::vector<Move> ChessPiecesLogic::get_queen_moves(const Grid& grid, Position pos) const
  {
    // TODO: Implement queen move logic
    return {};
  }

  std::vector<Move> ChessPiecesLogic::get_king_moves(const Grid& grid, Position pos) const
  {
    // TODO: Implement king move logic
    return {};
  }

  std::vector<Move> ChessPiecesLogic::get_sliding_moves(const Grid&                             grid,
                                                        Position                                pos,
                                                        const std::vector<std::pair<int, int>>& directions) const
  {
    // TODO: Implement sliding move logic (for bishops, rooks, queens)
    return {};
  }

} // namespace Chess
