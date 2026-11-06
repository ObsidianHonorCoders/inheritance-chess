/// @file      pieces_logic.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Pieces Logic (CPL).
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements piece move generation and attack detection.

#include "pieces_logic.h"
#include "chess_pawn.h"
#include "chess_knight.h"
#include "chess_bishop.h"
#include "chess_rook.h"
#include "chess_queen.h"
#include "chess_king.h"
#include <array>

namespace
{
  constexpr int BOARD_SIZE = 8;

  int square_index(const Chess::Position& pos)
  {
    return pos.rank * BOARD_SIZE + pos.file;
  }

  Chess::Position index_to_position(int idx)
  {
    return Chess::Position(idx % BOARD_SIZE, idx / BOARD_SIZE);
  }

  std::array<uint64_t, 64> make_knight_source_masks()
  {
    std::array<uint64_t, 64> masks{};
    constexpr std::array<std::pair<int, int>, 8> offsets{{
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1},
    }};

    for (int rank = 0; rank < BOARD_SIZE; ++rank)
    {
      for (int file = 0; file < BOARD_SIZE; ++file)
      {
        uint64_t mask = 0;
        for (const auto& [df, dr] : offsets)
        {
          const int sf = file + df;
          const int sr = rank + dr;
          if (sf >= 0 && sf < BOARD_SIZE && sr >= 0 && sr < BOARD_SIZE)
          {
            mask |= (uint64_t{1} << (sr * BOARD_SIZE + sf));
          }
        }
        masks[rank * BOARD_SIZE + file] = mask;
      }
    }

    return masks;
  }

  std::array<uint64_t, 64> make_king_source_masks()
  {
    std::array<uint64_t, 64> masks{};
    for (int rank = 0; rank < BOARD_SIZE; ++rank)
    {
      for (int file = 0; file < BOARD_SIZE; ++file)
      {
        uint64_t mask = 0;
        for (int dr = -1; dr <= 1; ++dr)
        {
          for (int df = -1; df <= 1; ++df)
          {
            if (df == 0 && dr == 0)
            {
              continue;
            }

            const int sf = file + df;
            const int sr = rank + dr;
            if (sf >= 0 && sf < BOARD_SIZE && sr >= 0 && sr < BOARD_SIZE)
            {
              mask |= (uint64_t{1} << (sr * BOARD_SIZE + sf));
            }
          }
        }
        masks[rank * BOARD_SIZE + file] = mask;
      }
    }

    return masks;
  }

  std::array<std::array<uint64_t, 64>, 2> make_pawn_source_masks()
  {
    std::array<std::array<uint64_t, 64>, 2> masks{};

    for (int rank = 0; rank < BOARD_SIZE; ++rank)
    {
      for (int file = 0; file < BOARD_SIZE; ++file)
      {
        uint64_t white_sources = 0;
        uint64_t black_sources = 0;

        // White pawn attacks upward (+rank), so sources to attack (file, rank) are (fileÂ±1, rank-1).
        const int white_src_rank = rank - 1;
        if (white_src_rank >= 0)
        {
          if (file - 1 >= 0)
          {
            white_sources |= (uint64_t{1} << (white_src_rank * BOARD_SIZE + (file - 1)));
          }
          if (file + 1 < BOARD_SIZE)
          {
            white_sources |= (uint64_t{1} << (white_src_rank * BOARD_SIZE + (file + 1)));
          }
        }

        // Black pawn attacks downward (-rank), so sources to attack (file, rank) are (fileÂ±1, rank+1).
        const int black_src_rank = rank + 1;
        if (black_src_rank < BOARD_SIZE)
        {
          if (file - 1 >= 0)
          {
            black_sources |= (uint64_t{1} << (black_src_rank * BOARD_SIZE + (file - 1)));
          }
          if (file + 1 < BOARD_SIZE)
          {
            black_sources |= (uint64_t{1} << (black_src_rank * BOARD_SIZE + (file + 1)));
          }
        }

        const int idx = rank * BOARD_SIZE + file;
        masks[0][idx] = white_sources;
        masks[1][idx] = black_sources;
      }
    }

    return masks;
  }

  bool has_piece_type_in_mask(const Chess::Grid& grid,
                              uint64_t           candidate_mask,
                              uint64_t           attacker_occupancy,
                              Chess::Color       by_color,
                              Chess::PieceType   piece_type)
  {
    const uint64_t candidates = candidate_mask & attacker_occupancy;
    if (candidates == 0)
    {
      return false;
    }

    for (int idx = 0; idx < 64; ++idx)
    {
      const uint64_t bit = (uint64_t{1} << idx);
      if ((candidates & bit) == 0)
      {
        continue;
      }

      const auto piece = grid.get_piece(index_to_position(idx));
      if (piece.has_value() && piece->color == by_color && piece->type == piece_type)
      {
        return true;
      }
    }

    return false;
  }

  bool has_slider_attacker_on_ray(const Chess::Grid& grid,
                                  Chess::Position    square,
                                  Chess::Color       by_color,
                                  int                file_step,
                                  int                rank_step,
                                  Chess::PieceType   primary_type,
                                  Chess::PieceType   secondary_type)
  {
    int file = square.file + file_step;
    int rank = square.rank + rank_step;

    while (file >= 0 && file < BOARD_SIZE && rank >= 0 && rank < BOARD_SIZE)
    {
      const Chess::Position probe(file, rank);
      if (!grid.is_occupied(probe))
      {
        file += file_step;
        rank += rank_step;
        continue;
      }

      const auto piece = grid.get_piece(probe);
      if (!piece.has_value())
      {
        file += file_step;
        rank += rank_step;
        continue;
      }

      if (piece->color == by_color && (piece->type == primary_type || piece->type == secondary_type))
      {
        return true;
      }

      return false; // First blocker is not an attacking slider.
    }

    return false;
  }

  void gather_other_pieces_from_cache(Chess::Position origin,
                                      const Chess::PositionList& all_positions,
                                      const Chess::ColorList&    all_colors,
                                      Chess::PositionList&       other_positions,
                                      Chess::ColorList&          other_colors)
  {
    for (size_t i = 0; i < all_positions.size() && i < all_colors.size(); ++i)
    {
      if (all_positions[i] != origin)
      {
        other_positions.push_back(all_positions[i]);
        other_colors.push_back(all_colors[i]);
      }
    }
  }

  std::vector<Chess::Move> convert_positions_to_moves(Chess::Position start,
                                                      const Chess::PositionList& generated_positions)
  {
    std::vector<Chess::Move> moves;
    moves.reserve(generated_positions.size());

    for (const auto& end_pos : generated_positions)
    {
      Chess::Move move;
      move.start_pos = start;
      move.end_pos   = end_pos;
      move.flags     = Chess::SpecialFlags::NONE;
      moves.push_back(move);
    }

    return moves;
  }

  template <typename PieceT>
  std::vector<Chess::Move> generate_piece_moves(const Chess::Grid&       grid,
                                                Chess::Position          pos,
                                                Chess::Color             color,
                                                const Chess::PositionList& all_positions,
                                                const Chess::ColorList&    all_colors)
  {
    PieceT piece(color, pos);

    Chess::PositionList other_positions;
    Chess::ColorList    other_colors;
    other_positions.reserve(all_positions.size());
    other_colors.reserve(all_colors.size());
    gather_other_pieces_from_cache(pos, all_positions, all_colors, other_positions, other_colors);

    Chess::PositionList generated_positions;
    piece.available_moves(generated_positions, other_positions, other_colors, grid);

    return convert_positions_to_moves(pos, generated_positions);
  }
} // namespace

namespace Chess
{

  /// @brief Get or build cached board data.
  void ChessPiecesLogic::get_or_build_cached_board_data(const Grid& grid,
                                                        const PositionList*& positions,
                                                        const ColorList*& colors) const
  {
    const uint64_t occupied = grid.occupancy_mask();
    const uint64_t white_occupied = grid.occupancy_mask(Color::WHITE);
    const uint64_t black_occupied = grid.occupancy_mask(Color::BLACK);

    const bool same_board = cache_valid_ && cache_occupied_ == occupied && cache_white_occupied_ == white_occupied
                            && cache_black_occupied_ == black_occupied;

    if (!same_board)
    {
      build_piece_cache(grid, cached_positions_, cached_colors_);
      cache_occupied_ = occupied;
      cache_white_occupied_ = white_occupied;
      cache_black_occupied_ = black_occupied;
      cache_valid_ = true;
    }

    positions = &cached_positions_;
    colors = &cached_colors_;
  }

  /// @brief Build piece cache.
  void ChessPiecesLogic::build_piece_cache(const Grid& grid, PositionList& positions, ColorList& colors) const
  {
    positions.clear();
    colors.clear();

    positions.reserve(32);
    colors.reserve(32);

    for (int file = 0; file < 8; ++file)
    {
      for (int rank = 0; rank < 8; ++rank)
      {
        Position pos(file, rank);
        if (!grid.is_occupied(pos))
        {
          continue;
        }

        const auto piece = grid.get_piece(pos);
        if (!piece.has_value())
        {
          continue;
        }

        positions.push_back(pos);
        colors.push_back(piece->color);
      }
    }
  }

  /// @brief Return whether raw logical move.
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

  /// @brief List raw logical moves.
  std::vector<Move> ChessPiecesLogic::list_raw_logical_moves(const Grid& grid, Position position) const
  {
    const PositionList* all_positions = nullptr;
    const ColorList* all_colors = nullptr;
    get_or_build_cached_board_data(grid, all_positions, all_colors);
    return list_raw_logical_moves_cached(grid, position, *all_positions, *all_colors);
  }

  /// @brief List raw logical moves cached.
  std::vector<Move> ChessPiecesLogic::list_raw_logical_moves_cached(const Grid&         grid,
                                                                     Position            position,
                                                                     const PositionList& positions,
                                                                     const ColorList&    colors) const
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
      return generate_piece_moves<ChessPawn>(grid, position, piece->color, positions, colors);
    case PieceType::KNIGHT:
      return generate_piece_moves<ChessKnight>(grid, position, piece->color, positions, colors);
    case PieceType::BISHOP:
      return generate_piece_moves<ChessBishop>(grid, position, piece->color, positions, colors);
    case PieceType::ROOK:
      return generate_piece_moves<ChessRook>(grid, position, piece->color, positions, colors);
    case PieceType::QUEEN:
      return generate_piece_moves<ChessQueen>(grid, position, piece->color, positions, colors);
    case PieceType::KING:
      return generate_piece_moves<ChessKing>(grid, position, piece->color, positions, colors);
    default:
      return {};
    }
  }

  /// @brief Return whether square attacked.
  bool ChessPiecesLogic::is_square_attacked(const Grid& grid, Position square, Color by_color) const
  {
    if (!square.is_valid())
    {
      return false;
    }

    static const std::array<uint64_t, 64> knight_sources = make_knight_source_masks();
    static const std::array<uint64_t, 64> king_sources   = make_king_source_masks();
    static const std::array<std::array<uint64_t, 64>, 2> pawn_sources = make_pawn_source_masks();

    const uint64_t attacker_occupancy = grid.occupancy_mask(by_color);
    if (attacker_occupancy == 0)
    {
      return false;
    }

    const int square_idx  = square_index(square);
    const int color_index = (by_color == Color::WHITE) ? 0 : 1;

    // Leaper/pawn attacks via precomputed source masks and side occupancy bitboard.
    if (has_piece_type_in_mask(grid, pawn_sources[color_index][square_idx], attacker_occupancy, by_color, PieceType::PAWN))
    {
      return true;
    }

    if (has_piece_type_in_mask(grid, knight_sources[square_idx], attacker_occupancy, by_color, PieceType::KNIGHT))
    {
      return true;
    }

    if (has_piece_type_in_mask(grid, king_sources[square_idx], attacker_occupancy, by_color, PieceType::KING))
    {
      return true;
    }

    // Slider attacks via directional rays from target square.
    if (has_slider_attacker_on_ray(grid, square, by_color, 1, 0, PieceType::ROOK, PieceType::QUEEN)
        || has_slider_attacker_on_ray(grid, square, by_color, -1, 0, PieceType::ROOK, PieceType::QUEEN)
        || has_slider_attacker_on_ray(grid, square, by_color, 0, 1, PieceType::ROOK, PieceType::QUEEN)
        || has_slider_attacker_on_ray(grid, square, by_color, 0, -1, PieceType::ROOK, PieceType::QUEEN))
    {
      return true;
    }

    if (has_slider_attacker_on_ray(grid, square, by_color, 1, 1, PieceType::BISHOP, PieceType::QUEEN)
        || has_slider_attacker_on_ray(grid, square, by_color, 1, -1, PieceType::BISHOP, PieceType::QUEEN)
        || has_slider_attacker_on_ray(grid, square, by_color, -1, 1, PieceType::BISHOP, PieceType::QUEEN)
        || has_slider_attacker_on_ray(grid, square, by_color, -1, -1, PieceType::BISHOP, PieceType::QUEEN))
    {
      return true;
    }

    return false;
  }

  /// @brief Get pawn moves.
  std::vector<Move> ChessPiecesLogic::get_pawn_moves(const Grid& grid, Position pos) const
  {
    const auto& piece_opt = grid.get_piece(pos);
    if (!piece_opt.has_value())
    {
      return {};
    }

    const auto& piece = piece_opt.value();
    const PositionList* all_positions = nullptr;
    const ColorList* all_colors = nullptr;
    get_or_build_cached_board_data(grid, all_positions, all_colors);
    return generate_piece_moves<ChessPawn>(grid, pos, piece.color, *all_positions, *all_colors);
  }

  /// @brief Get knight moves.
  std::vector<Move> ChessPiecesLogic::get_knight_moves(const Grid& grid, Position pos) const
  {
    const auto& piece_opt = grid.get_piece(pos);
    if (!piece_opt.has_value())
    {
      return {};
    }

    const auto& piece  = piece_opt.value();
    const PositionList* all_positions = nullptr;
    const ColorList* all_colors = nullptr;
    get_or_build_cached_board_data(grid, all_positions, all_colors);
    return generate_piece_moves<ChessKnight>(grid, pos, piece.color, *all_positions, *all_colors);
  }

  /// @brief Get bishop moves.
  std::vector<Move> ChessPiecesLogic::get_bishop_moves(const Grid& grid, Position pos) const
  {
    const auto& piece_opt = grid.get_piece(pos);
    if (!piece_opt.has_value())
    {
      return {};
    }

    const auto& piece  = piece_opt.value();
    const PositionList* all_positions = nullptr;
    const ColorList* all_colors = nullptr;
    get_or_build_cached_board_data(grid, all_positions, all_colors);
    return generate_piece_moves<ChessBishop>(grid, pos, piece.color, *all_positions, *all_colors);
  }

  /// @brief Get rook moves.
  std::vector<Move> ChessPiecesLogic::get_rook_moves(const Grid& grid, Position pos) const
  {
    const auto& piece_opt = grid.get_piece(pos);
    if (!piece_opt.has_value())
    {
      return {};
    }

    const auto& piece = piece_opt.value();
    const PositionList* all_positions = nullptr;
    const ColorList* all_colors = nullptr;
    get_or_build_cached_board_data(grid, all_positions, all_colors);
    return generate_piece_moves<ChessRook>(grid, pos, piece.color, *all_positions, *all_colors);
  }

  /// @brief Get queen moves.
  std::vector<Move> ChessPiecesLogic::get_queen_moves(const Grid& grid, Position pos) const
  {
    const auto& piece_opt = grid.get_piece(pos);
    if (!piece_opt.has_value())
    {
      return {};
    }

    const auto& piece = piece_opt.value();
    const PositionList* all_positions = nullptr;
    const ColorList* all_colors = nullptr;
    get_or_build_cached_board_data(grid, all_positions, all_colors);
    return generate_piece_moves<ChessQueen>(grid, pos, piece.color, *all_positions, *all_colors);
  }

  /// @brief Get king moves.
  std::vector<Move> ChessPiecesLogic::get_king_moves(const Grid& grid, Position pos) const
  {
    const auto& piece_opt = grid.get_piece(pos);
    if (!piece_opt.has_value())
    {
      return {};
    }

    const auto& piece = piece_opt.value();
    const PositionList* all_positions = nullptr;
    const ColorList* all_colors = nullptr;
    get_or_build_cached_board_data(grid, all_positions, all_colors);
    return generate_piece_moves<ChessKing>(grid, pos, piece.color, *all_positions, *all_colors);
  }

} // namespace Chess


