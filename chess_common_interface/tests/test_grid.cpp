/// @file      test_grid.cpp
/// @namespace Chess
/// @brief     Unit tests for Grid structure.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for Grid initialization, piece placement, and board state.

#include <gtest/gtest.h>
#include "../include/grid.h"

using namespace Chess;

class GridTest : public ::testing::Test
{
  protected:
    Grid grid;

    void SetUp() override { grid.initialize_standard_position(); }
};

TEST_F(GridTest, InitializeStandardPosition)
{
  // Check pawns
  EXPECT_TRUE(grid.is_occupied(Position(0, 1)));
  EXPECT_TRUE(grid.is_occupied(Position(0, 6)));

  // Check starting position
  EXPECT_TRUE(grid.is_occupied(Position(4, 0))); // White King
  EXPECT_TRUE(grid.is_occupied(Position(4, 7))); // Black King
}

TEST_F(GridTest, CheckCurrentTurn) { EXPECT_EQ(grid.current_turn, Color::WHITE); }

TEST_F(GridTest, SwitchTurn)
{
  EXPECT_EQ(grid.current_turn, Color::WHITE);
  grid.switch_turn();
  EXPECT_EQ(grid.current_turn, Color::BLACK);
}

TEST_F(GridTest, ClearSquare)
{
  Position pawn_pos(0, 1);
  EXPECT_TRUE(grid.is_occupied(pawn_pos));

  grid.clear_square(pawn_pos);
  EXPECT_FALSE(grid.is_occupied(pawn_pos));
}
