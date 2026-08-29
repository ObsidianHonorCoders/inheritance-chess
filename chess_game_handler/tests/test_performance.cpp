/// @file      test_performance.cpp
/// @namespace Chess
/// @brief     Performance tests for Chess Game Handler.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Benchmarks for move generation and game state calculations.

#include <gtest/gtest.h>
#include <chrono>
#include "../include/game_handler.h"

using namespace Chess;

class PerformanceTest : public ::testing::Test
{
  protected:
    ChessGameHandler handler;

    void SetUp() override { handler.start_new_game(); }
};

TEST_F(PerformanceTest, MoveGenerationPerformance)
{
  constexpr int iterations = 200;
  std::size_t total_moves = 0;

  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; ++i)
  {
    total_moves += handler.get_legal_moves().size();
  }
  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);

  EXPECT_GT(total_moves, 0u);
  // Keep threshold conservative to avoid flaky CI while still catching regressions.
  EXPECT_LT(elapsed_ms.count(), 2000);
}

TEST_F(PerformanceTest, GameStateCalculation)
{
  EXPECT_TRUE(handler.make_move("e2e4"));
  EXPECT_TRUE(handler.make_move("e7e5"));
  EXPECT_TRUE(handler.make_move("g1f3"));
  EXPECT_TRUE(handler.make_move("b8c6"));

  constexpr int iterations = 2000;
  int ongoing_count = 0;

  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; ++i)
  {
    if (handler.get_game_state() == Chess::GameState::ONGOING)
    {
      ++ongoing_count;
    }
  }
  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);

  EXPECT_EQ(ongoing_count, iterations);
  EXPECT_LT(elapsed_ms.count(), 2000);
}
