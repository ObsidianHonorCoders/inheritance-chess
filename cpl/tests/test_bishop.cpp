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

using namespace Chess;

class BishopMovesTest : public ::testing::Test
{
  protected:
    ChessPiecesLogic logic;

    void SetUp() override {}
};

TEST_F(BishopMovesTest, BishopDiagonalMoves)
{
  // TODO: Test bishop diagonal moves
}

TEST_F(BishopMovesTest, BishopBlockedByPiece)
{
  // TODO: Test bishop blocked by other pieces
}
