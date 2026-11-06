/// @file      test_chess_types.cpp
/// @namespace Chess
/// @brief     Unit tests for Chess Common Interface types and enumerations.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for Color, PieceType, and GameFlags enumerations.

#include <gtest/gtest.h>
#include "../include/chess_types.h"

using namespace Chess;

class ChessTypesTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
      // Setup any common test state here
    }
};

TEST_F(ChessTypesTest, ColorEnumExists)
{
  Color white = Color::WHITE;
  Color black = Color::BLACK;

  EXPECT_NE(white, black);
}

TEST_F(ChessTypesTest, PieceTypeEnumExists)
{
  PieceType pawn = PieceType::PAWN;
  PieceType king = PieceType::KING;

  EXPECT_NE(pawn, king);
}

TEST_F(ChessTypesTest, GameFlagsDefaultValues)
{
  GameFlags flags;

  EXPECT_TRUE(flags.white_can_castle_kingside);
  EXPECT_TRUE(flags.white_can_castle_queenside);
  EXPECT_TRUE(flags.black_can_castle_kingside);
  EXPECT_TRUE(flags.black_can_castle_queenside);
  EXPECT_EQ(flags.halfmove_clock, 0);
  EXPECT_EQ(flags.fullmove_number, 1);
}
