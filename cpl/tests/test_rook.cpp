/// @file      test_rook.cpp
/// @namespace Chess
/// @brief     Unit tests for rook move logic.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for rook straight-line movement and blocking.

#include <gtest/gtest.h>
#include "../include/pieces_logic.h"

using namespace Chess;

class RookMovesTest : public ::testing::Test
{
  protected:
    ChessPiecesLogic logic;

    void SetUp() override {}
};

TEST_F(RookMovesTest, RookStraightMoves)
{
  // TODO: Test rook straight moves
}

TEST_F(RookMovesTest, RookBlockedByPiece)
{
  // TODO: Test rook blocked by other pieces
}
