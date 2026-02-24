/// @file      test_pawns.cpp
/// @brief     Unit tests for the Pawn class using Google Test framework.
/// @author    Calileus
/// @date      2026-02-24
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite for Pawn class functionality.
/// @note      Uses std::unique_ptr for automatic memory management
///            following modern C++ RAII principles.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "pawns.hpp"

/// @class   PawnTest
/// @brief   Test fixture class for Pawn unit tests.
/// @details Provides setup and teardown methods for test environment.
///          Uses smart pointers for automatic memory management.
class PawnTest : public ::testing::Test
{
  protected:
    /// @brief   Default board properties for testing.
    Properties default_props = {};

    /// @brief   Smart pointer to a white Pawn instance under test.
    /// @details Uses std::unique_ptr for automatic memory management.
    std::unique_ptr<Pawn> p_white_pawn;

    /// @brief   Smart pointer to a black Pawn instance under test.
    /// @details Uses std::unique_ptr for automatic memory management.
    std::unique_ptr<Pawn> p_black_pawn;

    /// @brief   Set up test environment before each test.
    /// @details Creates new Pawn instances using std::make_unique.
    void SetUp() override
    {
      p_white_pawn = std::make_unique<Pawn>('e', '2', Piece::Color::WHITE);
      p_black_pawn = std::make_unique<Pawn>('e', '7', Piece::Color::BLACK);
    }

    /// @brief   Clean up test environment after each test.
    /// @details Automatically releases Pawn instances using smart pointer reset.
    void TearDown() override
    {
      p_white_pawn.reset();
      p_black_pawn.reset();
    }
};
