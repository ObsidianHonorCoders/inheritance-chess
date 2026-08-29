/// @file      test_pieces_logic.cpp
/// @namespace Chess
/// @brief     Unit tests for Chess Pieces Logic.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for piece move generation and board interactions.

#include <gtest/gtest.h>
#include "../include/pieces_logic.h"
#include <algorithm>

using namespace Chess;

class PiecesLogicTest : public ::testing::Test
{
  protected:
    ChessPiecesLogic logic;
    Grid             grid;

    void SetUp() override { grid.initialize_standard_position(); }
};

TEST_F(PiecesLogicTest, PawnInitialMoves)
{
  const auto moves = logic.list_raw_logical_moves(grid, Position(4, 1));

  auto has_end = [&moves](int file, int rank)
  {
    return std::any_of(moves.begin(), moves.end(), [file, rank](const Move& m)
    {
      return m.end_pos.file == file && m.end_pos.rank == rank;
    });
  };

  EXPECT_EQ(moves.size(), 2);
  EXPECT_TRUE(has_end(4, 2));
  EXPECT_TRUE(has_end(4, 3));
}

TEST_F(PiecesLogicTest, KnightMoves)
{
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

TEST_F(PiecesLogicTest, SquareAttacked)
{
  for (int file = 0; file < 8; ++file)
  {
    for (int rank = 0; rank < 8; ++rank)
    {
      grid.set_piece(Position(file, rank), std::nullopt);
    }
  }

  grid.set_piece(Position(4, 7), PieceProperties{PieceType::ROOK, Color::BLACK, Position(4, 7), true, false});
  grid.set_piece(Position(0, 0), PieceProperties{PieceType::KING, Color::WHITE, Position(0, 0), true, false});

  EXPECT_TRUE(logic.is_square_attacked(grid, Position(4, 0), Color::BLACK));
  EXPECT_FALSE(logic.is_square_attacked(grid, Position(4, 0), Color::WHITE));
}
