/// @file      test_king.cpp
/// @namespace Chess
/// @brief     Unit tests for king move logic.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for king movement, castling, and safety.

#include <gtest/gtest.h>
#include "../include/pieces_logic.h"
#include <algorithm>

using namespace Chess;

class KingMovesTest : public ::testing::Test
{
  protected:
    ChessPiecesLogic logic;

    void SetUp() override {}
};

TEST_F(KingMovesTest, KingAdjacentMoves)
{
  Grid grid;
  grid.initialize_standard_position();
  for (int file = 0; file < 8; ++file)
  {
    for (int rank = 0; rank < 8; ++rank)
    {
      grid.set_piece(Position(file, rank), std::nullopt);
    }
  }

  grid.set_piece(Position(4, 3), PieceProperties{PieceType::KING, Color::WHITE, Position(4, 3), true, false});

  const auto moves = logic.list_raw_logical_moves(grid, Position(4, 3));
  EXPECT_EQ(moves.size(), 8);
}

TEST_F(KingMovesTest, KingsideCastling)
{
  Grid grid;
  grid.initialize_standard_position();
  for (int file = 0; file < 8; ++file)
  {
    for (int rank = 0; rank < 8; ++rank)
    {
      grid.set_piece(Position(file, rank), std::nullopt);
    }
  }

  grid.set_piece(Position(4, 0), PieceProperties{PieceType::KING, Color::WHITE, Position(4, 0), false, false});
  grid.set_piece(Position(7, 0), PieceProperties{PieceType::ROOK, Color::WHITE, Position(7, 0), false, false});
  grid.flags.white_can_castle_kingside = true;
  grid.flags.white_can_castle_queenside = false;

  const auto moves = logic.list_raw_logical_moves(grid, Position(4, 0));

  const bool has_kingside = std::any_of(moves.begin(), moves.end(), [](const Move& m)
  {
    return m.end_pos.file == 6 && m.end_pos.rank == 0;
  });

  EXPECT_TRUE(has_kingside);
}

TEST_F(KingMovesTest, QueensideCastling)
{
  Grid grid;
  grid.initialize_standard_position();
  for (int file = 0; file < 8; ++file)
  {
    for (int rank = 0; rank < 8; ++rank)
    {
      grid.set_piece(Position(file, rank), std::nullopt);
    }
  }

  grid.set_piece(Position(4, 0), PieceProperties{PieceType::KING, Color::WHITE, Position(4, 0), false, false});
  grid.set_piece(Position(0, 0), PieceProperties{PieceType::ROOK, Color::WHITE, Position(0, 0), false, false});
  grid.flags.white_can_castle_kingside = false;
  grid.flags.white_can_castle_queenside = true;

  const auto moves = logic.list_raw_logical_moves(grid, Position(4, 0));

  const bool has_queenside = std::any_of(moves.begin(), moves.end(), [](const Move& m)
  {
    return m.end_pos.file == 2 && m.end_pos.rank == 0;
  });

  EXPECT_TRUE(has_queenside);
}
