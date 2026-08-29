/// @file      test_queen.cpp
/// @namespace Chess
/// @brief     Unit tests for queen move logic.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for queen combined rook and bishop movement.

#include <gtest/gtest.h>
#include "../include/pieces_logic.h"

using namespace Chess;

class QueenMovesTest : public ::testing::Test
{
  protected:
    ChessPiecesLogic logic;

    void SetUp() override {}
};

TEST_F(QueenMovesTest, QueenCombinedMoves)
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

  grid.set_piece(Position(3, 3), PieceProperties{PieceType::QUEEN, Color::WHITE, Position(3, 3), true, false});

  const auto moves = logic.list_raw_logical_moves(grid, Position(3, 3));
  EXPECT_EQ(moves.size(), 27);
}
