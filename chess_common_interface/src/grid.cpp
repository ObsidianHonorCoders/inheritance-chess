/// @file      grid.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Common Interface Grid structure.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements grid initialization and management functions.

#include "grid.h"
#include "move.h"
#include <algorithm>
#include <cstdlib>

namespace Chess
{

  Grid::Grid()
  {
    square_to_piece_index_.fill(-1);
  }

  /// @brief Initialize grid to standard starting position.
  void Grid::initialize_standard_position()
  {
    pieces_.clear();

    // Set up pawns (rank 1 for white, rank 6 for black)
    for (int file = 0; file < 8; file++)
    {
      add_piece_record(PieceType::PAWN, Color::WHITE, Position{file, 1}, false, false);
      add_piece_record(PieceType::PAWN, Color::BLACK, Position{file, 6}, false, false);
    }

    // Helper lambda to setup back ranks
    auto setup_back_rank = [this](int rank, Color color)
    {
      add_piece_record(PieceType::ROOK, color, Position{0, rank}, false, false);
      add_piece_record(PieceType::KNIGHT, color, Position{1, rank}, false, false);
      add_piece_record(PieceType::BISHOP, color, Position{2, rank}, false, false);
      add_piece_record(PieceType::QUEEN, color, Position{3, rank}, false, false);
      add_piece_record(PieceType::KING, color, Position{4, rank}, false, false);
      add_piece_record(PieceType::BISHOP, color, Position{5, rank}, false, false);
      add_piece_record(PieceType::KNIGHT, color, Position{6, rank}, false, false);
      add_piece_record(PieceType::ROOK, color, Position{7, rank}, false, false);
    };

    setup_back_rank(0, Color::WHITE);
    setup_back_rank(7, Color::BLACK);

    current_turn = Color::WHITE;
    flags        = GameFlags{}; // Reset to default castling rights
    rebuild_indexes();
  }

  /// @brief Get piece.
  std::optional<PieceProperties> Grid::get_piece(const Position& pos) const
  {
    if (!pos.is_valid())
    {
      return std::nullopt;
    }

    int idx = square_to_piece_index_[square_index(pos)];
    if (idx < 0 || static_cast<size_t>(idx) >= pieces_.size())
    {
      return std::nullopt;
    }

    // PieceProperties is the authoritative source of truth
    return pieces_[static_cast<size_t>(idx)].properties;
  }

  /// @brief Return whether occupied.
  bool Grid::is_occupied(const Position& pos) const
  {
    if (!pos.is_valid())
    {
      return false;
    }
    return (occupied_mask_ & (1ULL << square_index(pos))) != 0;
  }

  /// @brief Implement clear square.
  void Grid::clear_square(const Position& pos)
  {
    if (!pos.is_valid())
    {
      return;
    }
    (void)remove_piece(pos);
  }

  /// @brief Get all pieces on the board as a PieceList (backward-compatibility OO wrapper).
  /// @return Vector of raw pointers to all pieces on the board.
  /// @details Creates temporary OO adapters from value-type storage for external consumers.
  ///          Internally, use get_all_piece_properties() for the authoritative value-type API.
  std::vector<ChessPiece*> Grid::get_all_pieces() const
  {
    std::vector<ChessPiece*> result;
    result.reserve(pieces_.size());
    
    // For backward compatibility, create temporary OO wrappers from value-types
    // NOTE: This creates temporary objects. Prefer get_all_piece_properties() internally.
    for (const auto& record : pieces_)
    {
      // Create a temporary polymorphic piece from the value-type properties
      auto piece = create_piece(record.properties.type, record.properties.color, record.properties.position);
      if (piece)
      {
        result.push_back(piece.release()); // Release for caller to manage (caveat: caller must delete!)
      }
    }
    
    return result;
  }

  /// @brief Get all piece properties on the board (authoritative value-type API).
  /// @return Vector of all PieceProperties on the board.
  std::vector<PieceProperties> Grid::get_all_piece_properties() const
  {
    std::vector<PieceProperties> result;
    result.reserve(pieces_.size());
    
    for (const auto& record : pieces_)
    {
      result.push_back(record.properties);
    }
    
    return result;
  }

  /// @brief Add a piece to the board (OO interface - converts to value-type internally).
  /// @param piece Unique pointer to the piece to add.
  /// @details Extracts piece properties and delegates to set_piece() for value-type storage.
  ///          This bridges the OO interface to internal value-type authority.
  void Grid::add_piece(std::unique_ptr<ChessPiece> piece)
  {
    if (!piece)
    {
      return;
    }

    PieceProperties props{piece->get_type(), piece->get_color(), piece->get_position(), false, false};
    set_piece(piece->get_position(), props);
  }

  /// @brief Remove a piece from the board.
  /// @param pos Position of the piece to remove.
  /// @return True if piece was removed, false if no piece at position.
  bool Grid::remove_piece(const Position& pos)
  {
    if (!pos.is_valid())
    {
      return false;
    }

    int idx = square_to_piece_index_[square_index(pos)];
    if (idx < 0 || static_cast<size_t>(idx) >= pieces_.size())
    {
      return false;
    }

    pieces_.erase(pieces_.begin() + idx);
    rebuild_indexes();
    return true;
  }

  /// @brief Get piece at a specific position (deprecated - use get_piece() instead).
  /// @param pos Position to check.
  /// @return Raw pointer to temporarily created OO wrapper, or nullptr if empty.
  /// @warning DEPRECATED: Returns temporary wrapper with limited lifetime.
  ///          Use get_piece() for value-type access (authoritative).
  ChessPiece* Grid::get_piece_at(const Position& pos)
  {
    if (!pos.is_valid())
    {
      return nullptr;
    }

    auto props_opt = get_piece(pos);
    if (!props_opt.has_value())
    {
      return nullptr;
    }

    // Create temporary OO wrapper from value-type
    // WARNING: Caller must not store this pointer beyond the call!
    auto piece = create_piece(props_opt->type, props_opt->color, pos);
    return piece.release(); // Release for caller - caveat: caller must delete!
  }
  
  /// @brief Get piece at a specific position - const version (deprecated).
  /// @param pos Position to check.
  /// @return Const raw pointer to temporarily created OO wrapper, or nullptr if empty.
  /// @warning DEPRECATED: Returns temporary wrapper with limited lifetime.
  ///          Use get_piece() for value-type access (authoritative).
  const ChessPiece* Grid::get_piece_at(const Position& pos) const
  {
    if (!pos.is_valid())
    {
      return nullptr;
    }

    auto props_opt = get_piece(pos);
    if (!props_opt.has_value())
    {
      return nullptr;
    }

    // Create temporary OO wrapper from value-type
    // WARNING: Caller must not store this pointer beyond the call!
    auto piece = create_piece(props_opt->type, props_opt->color, pos);
    return piece.release(); // Release for caller - caveat: caller must delete!
  }

  /// @brief Convert grid state to PieceProperties array for compatibility.
  /// @details Bridges the gap between PieceList and grid representation.
  void Grid::update_piece_properties()
  {
    rebuild_indexes();
  }

/// @brief Set piece.
void Grid::set_piece(const Position& pos, const std::optional<PieceProperties>& piece)
{
  if (!pos.is_valid())
  {
    return;
  }

  (void)remove_piece(pos);

  if (piece.has_value())
  {
    add_piece_record(piece->type, piece->color, pos, piece->has_moved, piece->en_passant_vulnerable);
  }

  rebuild_indexes();
}

/// @brief Create a deep copy of the current grid state.
Grid Grid::clone() const
{
  Grid copy;
  copy.current_turn = current_turn;
  copy.flags = flags;

  copy.pieces_.clear();
  copy.pieces_.reserve(pieces_.size());

  for (const auto& record : pieces_)
  {
    copy.add_piece_record(record.properties.type,
                          record.properties.color,
                          record.properties.position,
                          record.properties.has_moved,
                          record.properties.en_passant_vulnerable);
  }

  copy.rebuild_indexes();
  return copy;
}

/// @brief Apply move inplace.
UndoRecord Grid::apply_move_inplace(const Move& move)
{
  UndoRecord undo;
  undo.turn_before = current_turn;
  undo.flags_before = flags;
  undo.en_passant_vulnerable_before.fill(false);

  for (const auto& record : pieces_)
  {
    const Position pos = record.properties.position;
    if (!pos.is_valid())
    {
      continue;
    }

    undo.en_passant_vulnerable_before[square_index(pos)] = record.properties.en_passant_vulnerable;
  }

  if (!move.start_pos.is_valid() || !move.end_pos.is_valid())
  {
    return undo;
  }

  auto moving_piece_opt = get_piece(move.start_pos);
  if (!moving_piece_opt.has_value())
  {
    return undo;
  }

  const PieceProperties moving_piece = moving_piece_opt.value();

  undo.captured_piece = get_piece(move.end_pos);

  if (move.is_castling())
  {
    clear_square(move.start_pos);
    set_piece(
        move.end_pos,
        PieceProperties{PieceType::KING, moving_piece.color, move.end_pos, true, false});

    const int rank = move.start_pos.rank;
    Position rook_start;
    Position rook_end;
    if (move.flags == SpecialFlags::CASTLE_KINGSIDE)
    {
      rook_start = Position(7, rank);
      rook_end = Position(5, rank);
    }
    else
    {
      rook_start = Position(0, rank);
      rook_end = Position(3, rank);
    }

    undo.rook_original_pos = rook_start;
    auto rook_opt = get_piece(rook_start);
    if (rook_opt.has_value())
    {
      clear_square(rook_start);
      clear_square(rook_end);
      set_piece(
          rook_end,
          PieceProperties{PieceType::ROOK, moving_piece.color, rook_end, true, false});
    }
  }
  else if (move.is_en_passant())
  {
    Position captured_pawn_pos(move.end_pos.file, move.start_pos.rank);
    undo.en_passant_pawn = get_piece(captured_pawn_pos);
    clear_square(move.start_pos);
    clear_square(captured_pawn_pos);
    set_piece(
        move.end_pos,
        PieceProperties{PieceType::PAWN, moving_piece.color, move.end_pos, true, false});
  }
  else if (move.is_promotion())
  {
    clear_square(move.start_pos);
    set_piece(
        move.end_pos,
        PieceProperties{move.promotion_piece, moving_piece.color, move.end_pos, true, false});
  }
  else
  {
    clear_square(move.start_pos);
    set_piece(
        move.end_pos,
        PieceProperties{moving_piece.type, moving_piece.color, move.end_pos, true, false});
  }

  // Reset halfmove clock on pawn moves or any capture (50-move rule).
  if (moving_piece.type == PieceType::PAWN || undo.captured_piece.has_value() || undo.en_passant_pawn.has_value())
  {
    flags.halfmove_clock = 0;
  }
  else
  {
    flags.halfmove_clock++;
  }

  if (moving_piece.color == Color::BLACK)
  {
    flags.fullmove_number++;
  }

  // Clear all en passant vulnerability from all pieces.
  for (auto& record : pieces_)
  {
    record.properties.en_passant_vulnerable = false;
  }

  // Set en passant vulnerability if pawn just made a two-square move from starting rank.
  if (moving_piece.type == PieceType::PAWN && std::abs(move.end_pos.rank - move.start_pos.rank) == 2)
  {
    const int idx = square_to_piece_index_[square_index(move.end_pos)];
    if (idx >= 0 && idx < static_cast<int>(pieces_.size()))
    {
      pieces_[idx].properties.en_passant_vulnerable = true;
    }
  }

  if (moving_piece.type == PieceType::KING)
  {
    if (moving_piece.color == Color::WHITE)
    {
      flags.white_can_castle_kingside = false;
      flags.white_can_castle_queenside = false;
    }
    else
    {
      flags.black_can_castle_kingside = false;
      flags.black_can_castle_queenside = false;
    }
  }

  if (moving_piece.type == PieceType::ROOK)
  {
    if (moving_piece.color == Color::WHITE)
    {
      if (move.start_pos.file == 0 && move.start_pos.rank == 0)
      {
        flags.white_can_castle_queenside = false;
      }
      if (move.start_pos.file == 7 && move.start_pos.rank == 0)
      {
        flags.white_can_castle_kingside = false;
      }
    }
    else
    {
      if (move.start_pos.file == 0 && move.start_pos.rank == 7)
      {
        flags.black_can_castle_queenside = false;
      }
      if (move.start_pos.file == 7 && move.start_pos.rank == 7)
      {
        flags.black_can_castle_kingside = false;
      }
    }
  }

  switch_turn();
  return undo;
}

/// @brief Undo move.
void Grid::undo_move(const Move& move, const UndoRecord& undo_record)
{
  if (!move.start_pos.is_valid() || !move.end_pos.is_valid())
  {
    return;
  }

  auto moving_piece_opt = get_piece(move.end_pos);
  if (!moving_piece_opt.has_value())
  {
    return;
  }

  const PieceProperties moving_piece = moving_piece_opt.value();

  if (move.is_castling())
  {
    clear_square(move.end_pos);
    set_piece(
        move.start_pos,
        PieceProperties{PieceType::KING, moving_piece.color, move.start_pos, false, false});

    const int rank = move.start_pos.rank;
    Position rook_end;
    if (move.flags == SpecialFlags::CASTLE_KINGSIDE)
    {
      rook_end = Position(5, rank);
    }
    else
    {
      rook_end = Position(3, rank);
    }

    auto rook_opt = get_piece(rook_end);
    if (rook_opt.has_value())
    {
      clear_square(rook_end);
      set_piece(
          undo_record.rook_original_pos,
          PieceProperties{PieceType::ROOK, moving_piece.color, undo_record.rook_original_pos, false, false});
    }
  }
  else if (move.is_en_passant())
  {
    clear_square(move.end_pos);
    set_piece(
        move.start_pos,
        PieceProperties{PieceType::PAWN, moving_piece.color, move.start_pos, false, false});

    if (undo_record.en_passant_pawn.has_value())
    {
      set_piece(Position(move.end_pos.file, move.start_pos.rank), undo_record.en_passant_pawn);
    }
  }
  else if (move.is_promotion())
  {
    clear_square(move.end_pos);
    set_piece(
        move.start_pos,
        PieceProperties{PieceType::PAWN, moving_piece.color, move.start_pos, false, false});

    if (undo_record.captured_piece.has_value())
    {
      set_piece(move.end_pos, undo_record.captured_piece);
    }
  }
  else
  {
    clear_square(move.end_pos);
    set_piece(
        move.start_pos,
        PieceProperties{moving_piece.type, moving_piece.color, move.start_pos, false, false});

    if (undo_record.captured_piece.has_value())
    {
      set_piece(move.end_pos, undo_record.captured_piece);
    }
  }

  current_turn = undo_record.turn_before;
  flags = undo_record.flags_before;

  for (auto& record : pieces_)
  {
    record.properties.en_passant_vulnerable = false;
  }

  for (size_t i = 0; i < pieces_.size(); ++i)
  {
    const Position pos = pieces_[i].properties.position;
    if (!pos.is_valid())
    {
      continue;
    }

    pieces_[i].properties.en_passant_vulnerable = undo_record.en_passant_vulnerable_before[square_index(pos)];
  }
}

/// @brief Implement square index.
int Grid::square_index(const Position& pos) const
{
  return pos.rank * 8 + pos.file;
}

/// @brief Implement rebuild indexes.
void Grid::rebuild_indexes()
{
  square_to_piece_index_.fill(-1);
  occupied_mask_ = 0;
  white_occupied_mask_ = 0;
  black_occupied_mask_ = 0;

  for (size_t i = 0; i < pieces_.size(); ++i)
  {
    const auto& record = pieces_[i];
    const Position pos = record.properties.position;
    if (!pos.is_valid())
    {
      continue;
    }

    const int sq = square_index(pos);
    square_to_piece_index_[sq] = static_cast<int>(i);

    const uint64_t bit = (1ULL << sq);
    occupied_mask_ |= bit;
    if (record.properties.color == Color::WHITE)
    {
      white_occupied_mask_ |= bit;
    }
    else
    {
      black_occupied_mask_ |= bit;
    }
  }
}

/// @brief Implement add piece record.
void Grid::add_piece_record(PieceType type,
                           Color color,
                           const Position& pos,
                           bool has_moved,
                           bool en_passant_vulnerable)
{
  // Store value-type properties directly (no polymorphic objects needed internally)
  PieceProperties props{type, color, pos, has_moved, en_passant_vulnerable};
  pieces_.push_back(PieceRecord{props});
}

/// @brief Set has moved.
void Grid::set_has_moved(const Position& pos)
{
  if (!pos.is_valid())
  {
    return;
  }

  int idx = square_to_piece_index_[square_index(pos)];
  if (idx >= 0 && static_cast<size_t>(idx) < pieces_.size())
  {
    pieces_[static_cast<size_t>(idx)].properties.has_moved = true;
  }
}

/// @brief Set en passant vulnerable.
void Grid::set_en_passant_vulnerable(const Position& pos)
{
  if (!pos.is_valid())
  {
    return;
  }

  int idx = square_to_piece_index_[square_index(pos)];
  if (idx >= 0 && static_cast<size_t>(idx) < pieces_.size())
  {
    pieces_[static_cast<size_t>(idx)].properties.en_passant_vulnerable = true;
  }
}

/// @brief Get en passant target.
Position Grid::get_en_passant_target() const
{
  // Search for a piece marked as en passant vulnerable
  for (const auto& record : pieces_)
  {
    if (record.properties.en_passant_vulnerable && record.properties.type == PieceType::PAWN)
    {
      const Position pawn_pos = record.properties.position;
      // En passant target is one rank forward from the vulnerable pawn
      // For white pawn (e.g. e4 after e2e4), target is one rank behind at e3.
      // For black pawn (e.g. d5 after d7d5), target is one rank ahead at d6.
      int target_rank = (record.properties.color == Color::WHITE) ? pawn_pos.rank - 1 : pawn_pos.rank + 1;
      if (target_rank >= 0 && target_rank <= 7)
      {
        return Position(pawn_pos.file, target_rank);
      }
    }
  }
  return Position(-1, -1); // Return invalid position if no en passant target
}

} // namespace Chess


