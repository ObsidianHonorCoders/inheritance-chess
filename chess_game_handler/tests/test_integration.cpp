/// @file      test_integration.cpp
/// @namespace Chess
/// @brief     Integration tests for Chess Game Handler.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for complete module interactions and game flow.

#include <gtest/gtest.h>
#include "../include/game_handler.h"

using namespace Chess;

class IntegrationTest : public ::testing::Test
{
  protected:
    ChessGameHandler handler;

    void SetUp() override { handler.start_new_game(); }
};

TEST_F(IntegrationTest, SimpleGameFlow)
{
  ASSERT_EQ(handler.get_game_state(), Chess::GameState::ONGOING);

  EXPECT_TRUE(handler.make_move("e2e4"));
  EXPECT_TRUE(handler.make_move("e7e5"));
  EXPECT_TRUE(handler.make_move("g1f3"));

  const auto white_pawn_e4 = handler.get_current_grid().get_piece(Position(4, 3));
  ASSERT_TRUE(white_pawn_e4.has_value());
  EXPECT_EQ(white_pawn_e4->type, PieceType::PAWN);
  EXPECT_EQ(white_pawn_e4->color, Color::WHITE);

  const auto black_pawn_e5 = handler.get_current_grid().get_piece(Position(4, 4));
  ASSERT_TRUE(black_pawn_e5.has_value());
  EXPECT_EQ(black_pawn_e5->type, PieceType::PAWN);
  EXPECT_EQ(black_pawn_e5->color, Color::BLACK);

  const auto white_knight_f3 = handler.get_current_grid().get_piece(Position(5, 2));
  ASSERT_TRUE(white_knight_f3.has_value());
  EXPECT_EQ(white_knight_f3->type, PieceType::KNIGHT);
  EXPECT_EQ(white_knight_f3->color, Color::WHITE);

  EXPECT_EQ(handler.get_game_state(), Chess::GameState::ONGOING);
}

TEST_F(IntegrationTest, GameEndConditions)
{
  // Draw transition via fifty-move rule.
  handler.load_game_from_fen("4k3/8/8/8/8/8/8/4KR2 w - - 100 1");
  ASSERT_TRUE(handler.make_move("f1f2"));
  EXPECT_EQ(handler.get_game_state(), Chess::GameState::DRAW_FIFTY_MOVE_RULE);

  // Stalemate-like end condition: no legal moves for side to move.
  handler.load_game_from_fen("7k/7P/6K1/8/8/8/8/8 b - - 0 1");
  EXPECT_TRUE(handler.get_legal_moves().empty());
  EXPECT_FALSE(handler.make_move("h8h7"));
}

TEST_F(IntegrationTest, ThreefoldRepetitionEndCondition)
{
  EXPECT_TRUE(handler.make_move("g1f3"));
  EXPECT_TRUE(handler.make_move("g8f6"));
  EXPECT_TRUE(handler.make_move("f3g1"));
  EXPECT_TRUE(handler.make_move("f6g8"));

  EXPECT_TRUE(handler.make_move("g1f3"));
  EXPECT_TRUE(handler.make_move("g8f6"));
  EXPECT_TRUE(handler.make_move("f3g1"));
  EXPECT_TRUE(handler.make_move("f6g8"));

  EXPECT_EQ(handler.get_game_state(), Chess::GameState::DRAW_THREEFOLD_REPETITION);
}
