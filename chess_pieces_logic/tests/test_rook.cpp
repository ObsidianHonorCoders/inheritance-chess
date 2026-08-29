/// @file      test_rook.cpp
/// @namespace Chess
/// @brief     Unit tests for rook move logic.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for rook straight-line movement and blocking.

#include <gtest/gtest.h>
#include "../include/pieces_logic.h"
#include <algorithm>

using namespace Chess;

class RookMovesTest : public ::testing::Test
{
  protected:
    ChessPiecesLogic logic;

    void SetUp() override {}
};

TEST_F(RookMovesTest, RookStraightMoves)
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

  grid.set_piece(Position(3, 3), PieceProperties{PieceType::ROOK, Color::WHITE, Position(3, 3), true, false});

  const auto moves = logic.list_raw_logical_moves(grid, Position(3, 3));
  EXPECT_EQ(moves.size(), 14);
}

TEST_F(RookMovesTest, RookBlockedByPiece)
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

  grid.set_piece(Position(3, 3), PieceProperties{PieceType::ROOK, Color::WHITE, Position(3, 3), true, false});
  grid.set_piece(Position(3, 5), PieceProperties{PieceType::PAWN, Color::WHITE, Position(3, 5), true, false});
  grid.set_piece(Position(1, 3), PieceProperties{PieceType::PAWN, Color::BLACK, Position(1, 3), true, false});

  const auto moves = logic.list_raw_logical_moves(grid, Position(3, 3));

  auto has_end = [&moves](int file, int rank)
  {
    return std::any_of(moves.begin(), moves.end(), [file, rank](const Move& m)
    {
      return m.end_pos.file == file && m.end_pos.rank == rank;
    });
  };

  EXPECT_TRUE(has_end(3, 4));
  EXPECT_FALSE(has_end(3, 5));
  EXPECT_FALSE(has_end(3, 6));
  EXPECT_TRUE(has_end(2, 3));
  EXPECT_TRUE(has_end(1, 3));
  EXPECT_FALSE(has_end(0, 3));
}
