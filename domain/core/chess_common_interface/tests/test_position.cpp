/// @file      test_position.cpp
/// @namespace Chess
/// @brief     Unit tests for Position structure.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for Position struct construction, validation, and operators.

#include <gtest/gtest.h>
#include "../include/position.h"

using namespace Chess;

class PositionTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
      // Setup any common test state here
    }
};

TEST_F(PositionTest, DefaultConstructor)
{
  Position pos;
  EXPECT_EQ(pos.file, 0);
  EXPECT_EQ(pos.rank, 0);
}

TEST_F(PositionTest, ConstructorWithValues)
{
  Position pos(4, 4);
  EXPECT_EQ(pos.file, 4);
  EXPECT_EQ(pos.rank, 4);
}

TEST_F(PositionTest, IsValidForValidPositions)
{
  Position pos(0, 0);
  EXPECT_TRUE(pos.is_valid());

  Position pos2(7, 7);
  EXPECT_TRUE(pos2.is_valid());
}

TEST_F(PositionTest, IsValidForInvalidPositions)
{
  Position pos(-1, 0);
  EXPECT_FALSE(pos.is_valid());

  Position pos2(8, 0);
  EXPECT_FALSE(pos2.is_valid());
}

TEST_F(PositionTest, EqualityOperator)
{
  Position pos1(4, 4);
  Position pos2(4, 4);
  Position pos3(5, 5);

  EXPECT_EQ(pos1, pos2);
  EXPECT_NE(pos1, pos3);
}
