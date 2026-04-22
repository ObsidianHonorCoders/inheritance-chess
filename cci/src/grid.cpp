/// @file      grid.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Common Interface Grid structure.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements grid initialization and management functions.

#include "grid.h"
#include <algorithm>

namespace Chess
{

  /// @brief Check if a square is attacked by a color.
  /// @note This is a placeholder. The actual implementation will be
  ///       provided by the ChessPiecesLogic (CPL) module.
  bool Grid::is_square_attacked_by(const Position& pos, Color attacking_color) const
  {
    // TODO: This will be implemented by CPL module
    // For now, return false
    return false;
  }

  /// @brief Initialize grid to standard starting position.
  void Grid::initialize_standard_position()
  {
    // Clear the board and piece list (adapted from existing Board initialization)
    pieces_.clear(); // Clear smart pointer list first
    
    for (int file = 0; file < 8; file++)
    {
      for (int rank = 0; rank < 8; rank++)
      {
        board[file][rank] = std::nullopt;
      }
    }

    // Set up pawns (rank 1 for white, rank 6 for black)
    // Note: In Phase 2, this will use the piece factory with smart pointers
    for (int file = 0; file < 8; file++)
    {
      board[file][1] = PieceProperties{PieceType::PAWN, Color::WHITE, Position{file, 1}, false, false};
      board[file][6] = PieceProperties{PieceType::PAWN, Color::BLACK, Position{file, 6}, false, false};
    }

    // Helper lambda to setup back ranks
    auto setup_back_rank = [this](int rank, Color color)
    {
      board[0][rank] = PieceProperties{PieceType::ROOK, color, Position{0, rank}};
      board[1][rank] = PieceProperties{PieceType::KNIGHT, color, Position{1, rank}};
      board[2][rank] = PieceProperties{PieceType::BISHOP, color, Position{2, rank}};
      board[3][rank] = PieceProperties{PieceType::QUEEN, color, Position{3, rank}};
      board[4][rank] = PieceProperties{PieceType::KING, color, Position{4, rank}};
      board[5][rank] = PieceProperties{PieceType::BISHOP, color, Position{5, rank}};
      board[6][rank] = PieceProperties{PieceType::KNIGHT, color, Position{6, rank}};
      board[7][rank] = PieceProperties{PieceType::ROOK, color, Position{7, rank}};
    };

    setup_back_rank(0, Color::WHITE);
    setup_back_rank(7, Color::BLACK);

    current_turn = Color::WHITE;
    flags        = GameFlags{}; // Reset to default castling rights
  }

  /// @brief Get all pieces on the board as a PieceList.
  /// @return Vector of unique pointers to all pieces on the board.
  /// @details Adapted from existing Board::pieces pattern using smart pointers.
  PieceList Grid::get_all_pieces() const
  {
    PieceList result;
    result.reserve(pieces_.size());
    
    // Create copies of the smart pointers (adapted from existing patterns)
    for (const auto& piece : pieces_)
    {
      if (piece)
      {
        // Note: In Phase 2, this will return actual piece objects
        // For now, this demonstrates the smart pointer pattern
        result.push_back(std::unique_ptr<ChessPiece>());
      }
    }
    
    return result;
  }

  /// @brief Add a piece to the board.
  /// @param piece Unique pointer to the piece to add.
  /// @details Takes ownership of the piece. Adapted from existing Board patterns.
  void Grid::add_piece(std::unique_ptr<ChessPiece> piece)
  {
    if (!piece)
      return;
      
    // Add to pieces list (smart pointer ownership pattern)
    pieces_.push_back(std::move(piece));
    
    // Update grid representation
    update_piece_properties();
  }

  /// @brief Remove a piece from the board.
  /// @param pos Position of the piece to remove.
  /// @return True if piece was removed, false if no piece at position.
  bool Grid::remove_piece(const Position& pos)
  {
    // Find piece at position (adapted from existing Board patterns)
    auto it = std::find_if(pieces_.begin(), pieces_.end(),
        [&pos](const std::unique_ptr<ChessPiece>& piece) {
            return piece && piece->get_position() == pos;
        });
    
    if (it != pieces_.end())
    {
      // Remove from pieces list (smart pointer automatic cleanup)
      pieces_.erase(it);
      
      // Update grid representation
      update_piece_properties();
      return true;
    }
    
    return false;
  }

  /// @brief Get piece at a specific position (if any).
  /// @param pos Position to check.
  /// @return Raw pointer to piece at position, or nullptr if empty.
  /// @details Adapted from existing Board piece access patterns.
  ChessPiece* Grid::get_piece_at(const Position& pos)
  {
    auto it = std::find_if(pieces_.begin(), pieces_.end(),
        [&pos](const std::unique_ptr<ChessPiece>& piece) {
            return piece && piece->get_position() == pos;
        });
    
    return (it != pieces_.end()) ? it->get() : nullptr;
  }
  
  /// @brief Get piece at a specific position (const version).
  /// @param pos Position to check.
  /// @return Const raw pointer to piece at position, or nullptr if empty.
  const ChessPiece* Grid::get_piece_at(const Position& pos) const
  {
    auto it = std::find_if(pieces_.begin(), pieces_.end(),
        [&pos](const std::unique_ptr<ChessPiece>& piece) {
            return piece && piece->get_position() == pos;
        });
    
    return (it != pieces_.end()) ? it->get() : nullptr;
  }

  /// @brief Convert grid state to PieceProperties array for compatibility.
  /// @details Bridges the gap between PieceList and grid representation.
  void Grid::update_piece_properties()
  {
    // Clear current board representation
    for (int file = 0; file < 8; file++)
    {
      for (int rank = 0; rank < 8; rank++)
      {
        board[file][rank] = std::nullopt;
      }
    }
    
    // Update board from pieces list (adapted from existing patterns)
    for (const auto& piece : pieces_)
    {
      if (piece)
      {
        Position pos = piece->get_position();
        if (pos.is_valid())
        {
          board[pos.file][pos.rank] = PieceProperties{
              piece->get_type(),
              piece->get_color(),
              pos,
              false, // has_moved - will be tracked properly in Phase 2
              false  // en_passant_vulnerable - will be tracked properly in Phase 2
          };
        }
      }
    }
  }

} // namespace Chess
