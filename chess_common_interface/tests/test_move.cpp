/// @file      test_move.cpp
/// @namespace Chess
/// @brief     Unit tests for Move structure.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for Move struct construction and special move detection.

#include <gtest/gtest.h>
#include "../include/move.h"

using namespace Chess;

class MoveTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
      // Setup any common test state here
    }
};

TEST_F(MoveTest, DefaultConstructor)
{
  Move move;
  EXPECT_EQ(move.start_pos, Position(0, 0));
  EXPECT_EQ(move.end_pos, Position(0, 0));
  EXPECT_EQ(move.flags, SpecialFlags::NONE);
}

TEST_F(MoveTest, ConstructorWithPositions)
{
  Position start(4, 1);
  Position end(4, 3);
  Move     move(start, end);

  EXPECT_EQ(move.start_pos, start);
  EXPECT_EQ(move.end_pos, end);
}

TEST_F(MoveTest, IsPromotionFlag)
{
  Move regular(Position(0, 0), Position(0, 1));
  EXPECT_FALSE(regular.is_promotion());

  Move promotion(Position(0, 0), Position(0, 1), SpecialFlags::PROMOTION);
  EXPECT_TRUE(promotion.is_promotion());
}

TEST_F(MoveTest, IsCastlingFlag)
{
  Move regular(Position(0, 0), Position(0, 1));
  EXPECT_FALSE(regular.is_castling());

  Move kingside(Position(4, 0), Position(6, 0), SpecialFlags::CASTLE_KINGSIDE);
  EXPECT_TRUE(kingside.is_castling());
}
