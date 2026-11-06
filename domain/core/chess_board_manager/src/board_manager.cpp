/// @file      board_manager.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Board Manager (CBM).
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements board management and game rule enforcement.

#include "board_manager.h"
#include "pieces_logic.h"
#include <algorithm>
#include <cmath>

namespace Chess
{

  /// @brief Return whether white king in check.
  bool ChessBoardManager::is_white_king_in_check(const Grid& grid) const
  {
    return is_king_in_check(grid, Color::WHITE);
  }

  /// @brief Return whether black king in check.
  bool ChessBoardManager::is_black_king_in_check(const Grid& grid) const
  {
    return is_king_in_check(grid, Color::BLACK);
  }

  /// @brief Return whether draw.
  bool ChessBoardManager::is_draw(const Grid& grid, Color color_turn) const
  {
    return get_draw_reason(grid, color_turn) != DrawReason::NONE;
  }

  /// @brief Return explicit draw reason from board-state rules.
  DrawReason ChessBoardManager::get_draw_reason(const Grid& grid, Color color_turn) const
  {
    (void)color_turn;
    // Draw condition 1: Fifty-move rule (50 halfmoves without capture or pawn move)
    if (grid.flags.halfmove_clock >= 100)
    {
      return DrawReason::FIFTY_MOVE_RULE;
    }

    struct MinorPieceInfo
    {
        Color     color;
        PieceType type;
        int       square_color; // 0 dark, 1 light
    };

    std::vector<MinorPieceInfo> minor_pieces;
    minor_pieces.reserve(4);

    for (int file = 0; file < 8; ++file)
    {
      for (int rank = 0; rank < 8; ++rank)
      {
        const auto piece_opt = grid.get_piece(Position(file, rank));
        if (!piece_opt.has_value())
        {
          continue;
        }

        const PieceProperties& piece = piece_opt.value();
        if (piece.type == PieceType::KING)
        {
          continue;
        }

        // Any pawn/rook/queen means mating material still exists.
        if (piece.type == PieceType::PAWN || piece.type == PieceType::ROOK || piece.type == PieceType::QUEEN)
        {
          return DrawReason::NONE;
        }

        minor_pieces.push_back({piece.color, piece.type, (file + rank) % 2});
      }
    }

    // K vs K
    if (minor_pieces.empty())
    {
      return DrawReason::INSUFFICIENT_MATERIAL;
    }

    // K+N vs K or K+B vs K
    if (minor_pieces.size() == 1)
    {
      return DrawReason::INSUFFICIENT_MATERIAL;
    }

    if (minor_pieces.size() == 2)
    {
      const auto& first = minor_pieces[0];
      const auto& second = minor_pieces[1];

      // K+N+N vs K
      if (first.type == PieceType::KNIGHT && second.type == PieceType::KNIGHT && first.color == second.color)
      {
        return DrawReason::INSUFFICIENT_MATERIAL;
      }

      // K+B vs K+B where bishops are on same-colored squares.
      if (first.type == PieceType::BISHOP && second.type == PieceType::BISHOP
          && first.square_color == second.square_color)
      {
        return DrawReason::INSUFFICIENT_MATERIAL;
      }
    }

    return DrawReason::NONE;
  }

  /// @brief Implement validate move.
  bool ChessBoardManager::validate_move(const Grid& grid, const Move& move) const
  {
    // Check if start position is valid and contains a piece
    if (!move.start_pos.is_valid() || !move.end_pos.is_valid())
    {
      return false;
    }

    const auto& piece_opt = grid.get_piece(move.start_pos);
    if (!piece_opt.has_value())
    {
      return false; // No piece at start position
    }

    const auto& piece_props = piece_opt.value();

    // Ensure special flags are only used with compatible piece types.
    if (move.flags == SpecialFlags::CASTLE_KINGSIDE || move.flags == SpecialFlags::CASTLE_QUEENSIDE)
    {
      if (piece_props.type != PieceType::KING)
      {
        return false;
      }
    }
    else if (move.flags == SpecialFlags::EN_PASSANT)
    {
      if (piece_props.type != PieceType::PAWN)
      {
        return false;
      }
    }
    else if (move.flags == SpecialFlags::PROMOTION)
    {
      if (piece_props.type != PieceType::PAWN)
      {
        return false;
      }
    }

    // Piece must belong to current player
    if (piece_props.color != grid.current_turn)
    {
      return false;
    }

    // Check if end position is either empty or contains opponent piece
    const auto& end_piece_opt = grid.get_piece(move.end_pos);
    if (end_piece_opt.has_value() && end_piece_opt->color == piece_props.color)
    {
      return false; // Can't capture own piece
    }

    // Validate piece-specific movement rules
    int file_diff = std::abs(move.end_pos.file - move.start_pos.file);
    int rank_diff = std::abs(move.end_pos.rank - move.start_pos.rank);

    switch (piece_props.type)
    {
    case PieceType::PAWN:
    {
      // Pawn moves forward based on color
      int direction = (piece_props.color == Color::WHITE) ? 1 : -1;
      int rank_move = move.end_pos.rank - move.start_pos.rank;

      const bool reaches_promotion_rank =
          (piece_props.color == Color::WHITE && move.end_pos.rank == 7)
          || (piece_props.color == Color::BLACK && move.end_pos.rank == 0);

      // Promotion must be explicit and use a valid promotion piece.
      if (move.is_promotion())
      {
        if (!reaches_promotion_rank)
        {
          return false;
        }

        if (move.promotion_piece == PieceType::PAWN || move.promotion_piece == PieceType::KING)
        {
          return false;
        }
      }
      else if (reaches_promotion_rank)
      {
        return false;
      }

      // En passant legality: only immediately after the opponent double-step and only
      // against an adjacent opponent pawn into an empty target square.
      if (move.is_en_passant())
      {
        if (rank_move != direction || file_diff != 1)
        {
          return false;
        }

        if (end_piece_opt.has_value())
        {
          return false;
        }

        if (grid.flags.halfmove_clock != 0)
        {
          return false;
        }

        const int required_start_rank = (piece_props.color == Color::WHITE) ? 4 : 3;
        if (move.start_pos.rank != required_start_rank)
        {
          return false;
        }

        const Position captured_pawn_pos(move.end_pos.file, move.start_pos.rank);
        const auto captured_pawn = grid.get_piece(captured_pawn_pos);
        if (!captured_pawn.has_value())
        {
          return false;
        }

        if (captured_pawn->type != PieceType::PAWN || captured_pawn->color == piece_props.color)
        {
          return false;
        }

        break;
      }

      // Must move in the correct direction
      if ((direction > 0 && rank_move <= 0) || (direction < 0 && rank_move >= 0))
      {
        return false;
      }

      // Check if it's a capture move
      if (end_piece_opt.has_value())
      {
        // Diagonal capture: move 1 square diagonally
        if (file_diff != 1 || std::abs(rank_move) != 1)
        {
          return false;
        }
      }
      else
      {
        // Non-capture: must move straight forward
        if (file_diff != 0)
        {
          return false;
        }

        // Can move 1 or 2 squares from starting position
        int max_move = (piece_props.has_moved) ? 1 : 2;
        if (std::abs(rank_move) > max_move || std::abs(rank_move) == 0)
        {
          return false;
        }

        // Check if path is clear for 2-square moves
        if (std::abs(rank_move) == 2)
        {
          Position mid(move.start_pos.file, move.start_pos.rank + direction);
          if (grid.is_occupied(mid))
          {
            return false;
          }
        }
      }
      break;
    }

    case PieceType::KNIGHT:
    {
      // Knight moves in L-shape: 2 squares in one direction, 1 in the other
      if (!((file_diff == 2 && rank_diff == 1) || (file_diff == 1 && rank_diff == 2)))
      {
        return false;
      }
      break;
    }

    case PieceType::BISHOP:
    {
      // Bishop moves diagonally
      if (file_diff != rank_diff || file_diff == 0)
      {
        return false;
      }
      // Check if path is clear
      int file_step = (move.end_pos.file > move.start_pos.file) ? 1 : -1;
      int rank_step = (move.end_pos.rank > move.start_pos.rank) ? 1 : -1;
      for (int f = move.start_pos.file + file_step, r = move.start_pos.rank + rank_step; f != move.end_pos.file;
           f += file_step, r += rank_step)
      {
        if (grid.is_occupied(Position(f, r)))
        {
          return false;
        }
      }
      break;
    }

    case PieceType::ROOK:
    {
      // Rook moves horizontally or vertically
      if ((file_diff == 0 && rank_diff == 0) || (file_diff != 0 && rank_diff != 0))
      {
        return false;
      }
      // Check if path is clear
      if (file_diff > 0)
      {
        int step = (move.end_pos.file > move.start_pos.file) ? 1 : -1;
        for (int f = move.start_pos.file + step; f != move.end_pos.file; f += step)
        {
          if (grid.is_occupied(Position(f, move.start_pos.rank)))
          {
            return false;
          }
        }
      }
      if (rank_diff > 0)
      {
        int step = (move.end_pos.rank > move.start_pos.rank) ? 1 : -1;
        for (int r = move.start_pos.rank + step; r != move.end_pos.rank; r += step)
        {
          if (grid.is_occupied(Position(move.start_pos.file, r)))
          {
            return false;
          }
        }
      }
      break;
    }

    case PieceType::QUEEN:
    {
      // Queen moves like rook or bishop
      if (file_diff == 0 && rank_diff == 0)
      {
        return false;
      }
      if (file_diff != 0 && rank_diff != 0 && file_diff != rank_diff)
      {
        return false;
      }
      // Check diagonal path
      if (file_diff == rank_diff && file_diff != 0)
      {
        int file_step = (move.end_pos.file > move.start_pos.file) ? 1 : -1;
        int rank_step = (move.end_pos.rank > move.start_pos.rank) ? 1 : -1;
        for (int f = move.start_pos.file + file_step, r = move.start_pos.rank + rank_step; f != move.end_pos.file;
             f += file_step, r += rank_step)
        {
          if (grid.is_occupied(Position(f, r)))
          {
            return false;
          }
        }
      }
      // Check straight path
      if (file_diff > 0)
      {
        int step = (move.end_pos.file > move.start_pos.file) ? 1 : -1;
        for (int f = move.start_pos.file + step; f != move.end_pos.file; f += step)
        {
          if (grid.is_occupied(Position(f, move.start_pos.rank)))
          {
            return false;
          }
        }
      }
      if (rank_diff > 0)
      {
        int step = (move.end_pos.rank > move.start_pos.rank) ? 1 : -1;
        for (int r = move.start_pos.rank + step; r != move.end_pos.rank; r += step)
        {
          if (grid.is_occupied(Position(move.start_pos.file, r)))
          {
            return false;
          }
        }
      }
      break;
    }

    case PieceType::KING:
    {
      if (move.is_castling())
      {
        const bool kingside = (move.flags == SpecialFlags::CASTLE_KINGSIDE);
        const int  home_rank = (piece_props.color == Color::WHITE) ? 0 : 7;
        const int  expected_end_file = kingside ? 6 : 2;

        if (move.start_pos.file != 4 || move.start_pos.rank != home_rank || move.end_pos.rank != home_rank
            || move.end_pos.file != expected_end_file)
        {
          return false;
        }

        if (piece_props.has_moved)
        {
          return false;
        }

        const bool has_rights =
            (piece_props.color == Color::WHITE)
                ? (kingside ? grid.flags.white_can_castle_kingside : grid.flags.white_can_castle_queenside)
                : (kingside ? grid.flags.black_can_castle_kingside : grid.flags.black_can_castle_queenside);
        if (!has_rights)
        {
          return false;
        }

        const Position rook_start(kingside ? 7 : 0, home_rank);
        const auto rook = grid.get_piece(rook_start);
        if (!rook.has_value() || rook->type != PieceType::ROOK || rook->color != piece_props.color || rook->has_moved)
        {
          return false;
        }

        const int path_start = kingside ? 5 : 1;
        const int path_end = kingside ? 6 : 3;
        for (int file = path_start; file <= path_end; ++file)
        {
          if (grid.is_occupied(Position(file, home_rank)))
          {
            return false;
          }
        }

        if (!kingside && grid.is_occupied(Position(3, home_rank)))
        {
          return false;
        }

        if (is_king_in_check(grid, piece_props.color))
        {
          return false;
        }

        Grid test_grid = grid.clone();
        const int through_file = kingside ? 5 : 3;
        const Move through_move(move.start_pos, Position(through_file, home_rank));
        test_grid.apply_move_inplace(through_move);
        if (is_king_in_check(test_grid, piece_props.color))
        {
          return false;
        }

        test_grid = grid.clone();
        test_grid.apply_move_inplace(move);
        if (is_king_in_check(test_grid, piece_props.color))
        {
          return false;
        }

        return true;
      }

      if (file_diff > 1 || rank_diff > 1)
      {
        return false;
      }
      break;
    }
    }

    // Test if move would leave own king in check
    Grid test_grid = grid.clone();
    test_grid.apply_move_inplace(move);

    return !is_king_in_check(test_grid, piece_props.color);
  }

  /// @brief Return whether checkmate.
  bool ChessBoardManager::is_checkmate(const Grid& grid, Color color) const
  {
    if (!is_king_in_check(grid, color))
    {
      return false;
    }

    Grid turn_grid = grid.clone();
    turn_grid.current_turn = color;
    return get_legal_moves(turn_grid).empty();
  }

  /// @brief Return whether stalemate.
  bool ChessBoardManager::is_stalemate(const Grid& grid, Color color) const
  {
    if (is_king_in_check(grid, color))
    {
      return false;
    }

    Grid turn_grid = grid.clone();
    turn_grid.current_turn = color;
    return get_legal_moves(turn_grid).empty();
  }

  /// @brief Return whether king in check.
  bool ChessBoardManager::is_king_in_check(const Grid& grid, Color king_color) const
  {
    bool found_king = false;
    Position king_pos;

    for (int file = 0; file < 8 && !found_king; ++file)
    {
      for (int rank = 0; rank < 8; ++rank)
      {
        const auto piece = grid.get_piece(Position(file, rank));
        if (!piece.has_value())
        {
          continue;
        }

        if (piece->type == PieceType::KING && piece->color == king_color)
        {
          king_pos = Position(file, rank);
          found_king = true;
          break;
        }
      }
    }

    if (!found_king)
    {
      return false;
    }

    const Color opponent_color = (king_color == Color::WHITE) ? Color::BLACK : Color::WHITE;
    ChessPiecesLogic logic;
    return logic.is_square_attacked(grid, king_pos, opponent_color);
  }

  /// @brief Get legal moves.
  std::vector<Move> ChessBoardManager::get_legal_moves(const Grid& grid) const
  {
    std::vector<Move> legal_moves;
    ChessPiecesLogic pieces_logic;
    PositionList cached_positions;
    ColorList cached_colors;

    auto push_if_legal = [this, &grid, &legal_moves](const Move& candidate)
    {
      if (!validate_move(grid, candidate))
      {
        return;
      }

      if (std::find(legal_moves.begin(), legal_moves.end(), candidate) == legal_moves.end())
      {
        legal_moves.push_back(candidate);
      }
    };

    pieces_logic.build_piece_cache(grid, cached_positions, cached_colors);

    for (int file = 0; file < 8; ++file)
    {
      for (int rank = 0; rank < 8; ++rank)
      {
        const Position pos(file, rank);
        if (!grid.is_occupied(pos))
        {
          continue;
        }

        const auto piece = grid.get_piece(pos);
        if (!piece.has_value() || piece->color != grid.current_turn)
        {
          continue;
        }

        const auto raw_moves = pieces_logic.list_raw_logical_moves_cached(grid, pos, cached_positions, cached_colors);
        for (const auto& move : raw_moves)
        {
          if (move.flags != SpecialFlags::NONE)
          {
            push_if_legal(move);
            continue;
          }

          if (piece->type == PieceType::KING && move.start_pos.rank == move.end_pos.rank
              && std::abs(move.end_pos.file - move.start_pos.file) == 2)
          {
            const SpecialFlags castle_flag = (move.end_pos.file > move.start_pos.file)
                                                ? SpecialFlags::CASTLE_KINGSIDE
                                                : SpecialFlags::CASTLE_QUEENSIDE;
            push_if_legal(Move(move.start_pos, move.end_pos, castle_flag));
            continue;
          }

          if (piece->type == PieceType::PAWN)
          {
            const int promotion_rank = (piece->color == Color::WHITE) ? 7 : 0;
            const int file_delta = std::abs(move.end_pos.file - move.start_pos.file);
            const bool destination_occupied = grid.is_occupied(move.end_pos);

            if (move.end_pos.rank == promotion_rank)
            {
              push_if_legal(Move(move.start_pos, move.end_pos, SpecialFlags::PROMOTION, PieceType::QUEEN));
              push_if_legal(Move(move.start_pos, move.end_pos, SpecialFlags::PROMOTION, PieceType::ROOK));
              push_if_legal(Move(move.start_pos, move.end_pos, SpecialFlags::PROMOTION, PieceType::BISHOP));
              push_if_legal(Move(move.start_pos, move.end_pos, SpecialFlags::PROMOTION, PieceType::KNIGHT));
              continue;
            }

            if (file_delta == 1 && !destination_occupied)
            {
              push_if_legal(Move(move.start_pos, move.end_pos, SpecialFlags::EN_PASSANT));
              continue;
            }
          }

          push_if_legal(move);
        }
      }
    }

    return legal_moves;
  }

} // namespace Chess


