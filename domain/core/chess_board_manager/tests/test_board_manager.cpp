/// @file      test_board_manager.cpp
/// @namespace Chess
/// @brief     Unit tests for Chess Board Manager.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for board manager functionality including check detection.

#include <gtest/gtest.h>
#include "../include/board_manager.h"

using namespace Chess;

class BoardManagerTest : public ::testing::Test
{
  protected:
    ChessBoardManager manager;
    Grid              grid;

    void SetUp() override { grid.initialize_standard_position(); }
};

TEST_F(BoardManagerTest, InitialPositionNotInCheck)
{
  EXPECT_FALSE(manager.is_white_king_in_check(grid));
  EXPECT_FALSE(manager.is_black_king_in_check(grid));
}

TEST_F(BoardManagerTest, ValidateMoveInitialPawn)
{
  // Test valid initial pawn move: e2 to e4 (double-step from starting position)
  Move valid_pawn_move(Position(4, 1), Position(4, 3));
  EXPECT_TRUE(manager.validate_move(grid, valid_pawn_move));
  
  // Test invalid move: pawn backwards (e4 to e2) - should fail
  Move invalid_backward_move(Position(4, 3), Position(4, 1));
  EXPECT_FALSE(manager.validate_move(grid, invalid_backward_move));
  
  // Test invalid move: pawn two squares sideways - should fail
  Move invalid_sideways_move(Position(4, 1), Position(6, 1));
  EXPECT_FALSE(manager.validate_move(grid, invalid_sideways_move));
}
