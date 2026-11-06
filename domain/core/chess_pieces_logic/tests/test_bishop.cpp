/// @file      test_bishop.cpp
/// @namespace Chess
/// @brief     Unit tests for bishop move logic.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for bishop diagonal movement and blocking.

#include <gtest/gtest.h>
#include "../include/pieces_logic.h"
#include <algorithm>

using namespace Chess;

class BishopMovesTest : public ::testing::Test
{
  protected:
    ChessPiecesLogic logic;

    void SetUp() override {}
};

TEST_F(BishopMovesTest, BishopDiagonalMoves)
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

  grid.set_piece(Position(3, 3), PieceProperties{PieceType::BISHOP, Color::WHITE, Position(3, 3), true, false});

  const auto moves = logic.list_raw_logical_moves(grid, Position(3, 3));
  EXPECT_EQ(moves.size(), 13);
}

TEST_F(BishopMovesTest, BishopBlockedByPiece)
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

  grid.set_piece(Position(3, 3), PieceProperties{PieceType::BISHOP, Color::WHITE, Position(3, 3), true, false});
  grid.set_piece(Position(4, 4), PieceProperties{PieceType::PAWN, Color::WHITE, Position(4, 4), true, false});
  grid.set_piece(Position(1, 1), PieceProperties{PieceType::PAWN, Color::BLACK, Position(1, 1), true, false});

  const auto moves = logic.list_raw_logical_moves(grid, Position(3, 3));

  auto has_end = [&moves](int file, int rank)
  {
    return std::any_of(moves.begin(), moves.end(), [file, rank](const Move& m)
    {
      return m.end_pos.file == file && m.end_pos.rank == rank;
    });
  };

  EXPECT_TRUE(has_end(2, 2));
  EXPECT_TRUE(has_end(1, 1));
  EXPECT_FALSE(has_end(0, 0));
  EXPECT_FALSE(has_end(4, 4));
  EXPECT_FALSE(has_end(5, 5));
}
