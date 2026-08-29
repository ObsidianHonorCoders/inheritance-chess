/// @file      test_pawn.cpp
/// @namespace Chess
/// @brief     Unit tests for pawn move logic.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for pawn movement, captures, en passant, and promotion.

#include <gtest/gtest.h>
#include "../include/pieces_logic.h"
#include <algorithm>

using namespace Chess;

class PawnMovesTest : public ::testing::Test
{
  protected:
    ChessPiecesLogic logic;

    void SetUp() override {}
};

TEST_F(PawnMovesTest, PawnDoubleStartMove)
{
  Grid grid;
  grid.initialize_standard_position();

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

TEST_F(PawnMovesTest, PawnSingleMove)
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

  grid.set_piece(Position(4, 3), PieceProperties{PieceType::PAWN, Color::WHITE, Position(4, 3), true, false});

  const auto moves = logic.list_raw_logical_moves(grid, Position(4, 3));

  ASSERT_EQ(moves.size(), 1);
  EXPECT_EQ(moves[0].end_pos.file, 4);
  EXPECT_EQ(moves[0].end_pos.rank, 4);
}

TEST_F(PawnMovesTest, PawnCapture)
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

  grid.set_piece(Position(4, 3), PieceProperties{PieceType::PAWN, Color::WHITE, Position(4, 3), true, false});
  grid.set_piece(Position(3, 4), PieceProperties{PieceType::KNIGHT, Color::BLACK, Position(3, 4), true, false});
  grid.set_piece(Position(5, 4), PieceProperties{PieceType::BISHOP, Color::BLACK, Position(5, 4), true, false});

  const auto moves = logic.list_raw_logical_moves(grid, Position(4, 3));

  auto has_end = [&moves](int file, int rank)
  {
    return std::any_of(moves.begin(), moves.end(), [file, rank](const Move& m)
    {
      return m.end_pos.file == file && m.end_pos.rank == rank;
    });
  };

  EXPECT_EQ(moves.size(), 3);
  EXPECT_TRUE(has_end(4, 4));
  EXPECT_TRUE(has_end(3, 4));
  EXPECT_TRUE(has_end(5, 4));
}

TEST_F(PawnMovesTest, EnPassantCapture)
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

  grid.set_piece(Position(4, 4), PieceProperties{PieceType::PAWN, Color::WHITE, Position(4, 4), true, false});
  grid.set_piece(Position(3, 4), PieceProperties{PieceType::PAWN, Color::BLACK, Position(3, 4), true, true});
  grid.flags.halfmove_clock = 0;

  const auto moves = logic.list_raw_logical_moves(grid, Position(4, 4));

  auto has_end = [&moves](int file, int rank)
  {
    return std::any_of(moves.begin(), moves.end(), [file, rank](const Move& m)
    {
      return m.end_pos.file == file && m.end_pos.rank == rank;
    });
  };

  EXPECT_TRUE(has_end(4, 5));
  EXPECT_TRUE(has_end(3, 5));
}

TEST_F(PawnMovesTest, PawnPromotion)
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

  grid.set_piece(Position(4, 6), PieceProperties{PieceType::PAWN, Color::WHITE, Position(4, 6), true, false});

  const auto moves = logic.list_raw_logical_moves(grid, Position(4, 6));

  ASSERT_EQ(moves.size(), 1);
  EXPECT_EQ(moves[0].end_pos.file, 4);
  EXPECT_EQ(moves[0].end_pos.rank, 7);
}
