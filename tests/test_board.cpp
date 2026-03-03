/// @file      test_board.cpp
/// @brief     Unit tests for the Board class using Google Test framework.
/// @author    Calileus
/// @date      2026-02-09
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite for Board class functionality including:
///             - Board creation and initialization
///             - Smart pointer memory management
///             - Board display operations
///             - Standard chess setup validation
/// @note       Uses std::unique_ptr for automatic memory management
///             following modern C++ RAII principles.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "board.hpp"

/// @class   BoardTest
/// @brief   Test fixture class for Board unit tests.
/// @details Provides setup and teardown methods for test environment.
///          Uses smart pointers for automatic memory management.
class BoardTest : public ::testing::Test
{
  protected:
    /// @brief   Set up test environment before each test.
    /// @details Creates a new Board instance using std::make_unique.
    void SetUp() override { p_test_board = std::make_unique<Board>(); }

    /// @brief   Clean up test environment after each test.
    /// @details Automatically releases Board instance using smart pointer reset.
    void TearDown() override { p_test_board.reset(); }

    /// @brief   Smart pointer to the Board instance under test.
    /// @details Uses std::unique_ptr for automatic memory management.
    std::unique_ptr<Board> p_test_board;
};

/// @brief   Test board creation and smart pointer initialization.
/// @details Verifies that a new Board instance is properly created
///          and the smart pointer is not null after SetUp.
TEST_F(BoardTest, CreationState)
{
  // Expect a valid smart pointer from SetUp (Not Equal nullptr)
  EXPECT_NE(p_test_board.get(), nullptr);
  EXPECT_TRUE(p_test_board != nullptr);
  // Verify that the smart pointer owns a valid object
  EXPECT_TRUE(p_test_board != nullptr);
}

/// @brief   Test board display functionality with empty board.
/// @details Verifies that the board display method executes without errors
///          when called on an empty board instance.
TEST_F(BoardTest, DisplayEmptyBoard)
{
  // Test that display doesn't throw exceptions on empty board
  EXPECT_NO_THROW(p_test_board->display());
}

/// @brief   Test standard chess setup initialization and display.
/// @details Verifies that the board correctly initializes with standard
///          chess pieces and can display the setup without errors.
///          Tests both initialization and display functionality together.
TEST_F(BoardTest, DisplayStandardSetup)
{
  // The board should have 32 pieces in standard setup
  // This is a basic test - we can expand it to count pieces check specific positions
  // No exceptions are expected from initialization or display functions
  EXPECT_NO_THROW(p_test_board->initializeStandardSetup());
  EXPECT_NO_THROW(p_test_board->display());
}
