/// @file      test_knight.cpp
/// @namespace Chess
/// @brief     Unit tests for knight move logic.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for knight movement in various board positions.

#include <gtest/gtest.h>
#include "../include/pieces_logic.h"
#include <algorithm>

using namespace Chess;

class KnightMovesTest : public ::testing::Test
{
  protected:
    ChessPiecesLogic logic;

    void SetUp() override {}
};

TEST_F(KnightMovesTest, KnightStartingMoves)
{
  Grid grid;
  grid.initialize_standard_position();

  const auto moves = logic.list_raw_logical_moves(grid, Position(1, 0));

  auto has_end = [&moves](int file, int rank)
  {
    return std::any_of(moves.begin(), moves.end(), [file, rank](const Move& m)
    {
      return m.end_pos.file == file && m.end_pos.rank == rank;
    });
  };

  EXPECT_EQ(moves.size(), 2);
  EXPECT_TRUE(has_end(0, 2));
  EXPECT_TRUE(has_end(2, 2));
}

TEST_F(KnightMovesTest, KnightCenterMoves)
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

  grid.set_piece(Position(3, 3), PieceProperties{PieceType::KNIGHT, Color::WHITE, Position(3, 3), true, false});

  const auto moves = logic.list_raw_logical_moves(grid, Position(3, 3));
  EXPECT_EQ(moves.size(), 8);
}

TEST_F(KnightMovesTest, KnightEdgeMoves)
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

  grid.set_piece(Position(0, 0), PieceProperties{PieceType::KNIGHT, Color::WHITE, Position(0, 0), true, false});

  const auto moves = logic.list_raw_logical_moves(grid, Position(0, 0));

  auto has_end = [&moves](int file, int rank)
  {
    return std::any_of(moves.begin(), moves.end(), [file, rank](const Move& m)
    {
      return m.end_pos.file == file && m.end_pos.rank == rank;
    });
  };

  EXPECT_EQ(moves.size(), 2);
  EXPECT_TRUE(has_end(1, 2));
  EXPECT_TRUE(has_end(2, 1));
}
