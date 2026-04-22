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

using namespace Chess;

class PawnMovesTest : public ::testing::Test
{
  protected:
    ChessPiecesLogic logic;

    void SetUp() override {}
};

TEST_F(PawnMovesTest, PawnDoubleStartMove)
{
  // TODO: Test pawn double move from starting rank
}

TEST_F(PawnMovesTest, PawnSingleMove)
{
  // TODO: Test pawn single move
}

TEST_F(PawnMovesTest, PawnCapture)
{
  // TODO: Test pawn capture logic
}

TEST_F(PawnMovesTest, EnPassantCapture)
{
  // TODO: Test en passant capture
}

TEST_F(PawnMovesTest, PawnPromotion)
{
  // TODO: Test pawn promotion
}
