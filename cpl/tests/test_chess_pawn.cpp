/// @file      test_chess_pawn.cpp
/// @brief     Unit tests for ChessPawn class using Google Test framework.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite for ChessPawn class functionality adapted from existing test_pawns.cpp:
///             - ChessPawn creation and initialization
///             - Single and double step forward movement
///             - Diagonal capture moves
///             - En passant capture moves
/// @note      Uses std::unique_ptr for automatic memory management
///            following modern C++ RAII principles adapted from existing tests.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../include/chess_pawn.h"
#include "../../cci/include/grid.h"

/// @class   ChessPawnTest
/// @brief   Test fixture class for ChessPawn unit tests.
/// @details Provides setup and teardown methods for test environment.
///          Uses smart pointers for automatic memory management.
///          Adapted from existing PawnTest patterns.
class ChessPawnTest : public ::testing::Test
{
  protected:
    /// @brief   Grid instance for testing.
    /// @details Uses CCI Grid structure for board state.
    Chess::Grid test_grid;

    /// @brief   Available moves vector for test results.
    /// @details Populated by pawn's available_moves() method during testing.
    Chess::PositionList moves = {};

    /// @brief   Expected moves vector for test validation.
    /// @details Contains the positions we expect the pawn to be able to move to.
    Chess::PositionList expected_moves = {};

    /// @brief   Other pieces positions on the board for move validation.
    /// @details Represents positions of all other pieces during testing.
    Chess::PositionList other_pieces = {};

    /// @brief   Colors corresponding to other_pieces for capture validation.
    /// @details Used to determine which pieces can be captured.
    Chess::ColorList other_colors = {};

    /// @brief   Smart pointer to a white ChessPawn instance under test.
    /// @details Uses std::unique_ptr for automatic memory management.
    std::unique_ptr<Chess::ChessPawn> p_e2_white_pawn;

    /// @brief   Smart pointer to a black ChessPawn instance under test.
    /// @details Uses std::unique_ptr for automatic memory management.
    std::unique_ptr<Chess::ChessPawn> p_e7_black_pawn;

    /// @brief   Smart pointer to a white ChessPawn at e4 (mid-board position).
    /// @details Used for testing single-step movement from non-starting positions.
    std::unique_ptr<Chess::ChessPawn> p_e4_white_pawn;

    /// @brief   Smart pointer to a black ChessPawn at e5 (mid-board position).
    /// @details Used for testing single-step movement from non-starting positions.
    std::unique_ptr<Chess::ChessPawn> p_e5_black_pawn;

    /// @brief   Smart pointer to a white ChessPawn at e5 (en passant position).
    /// @details Used for testing en passant capture scenarios.
    std::unique_ptr<Chess::ChessPawn> p_e5_white_pawn;

    /// @brief   Smart pointer to a black ChessPawn at e4 (en passant position).
    /// @details Used for testing en passant capture scenarios.
    std::unique_ptr<Chess::ChessPawn> p_e4_black_pawn;

    /// @brief   Set up test environment before each test.
    /// @details Creates new ChessPawn instances at various positions for comprehensive testing:
    ///          - e2/e7: Starting positions for forward movement tests
    ///          - e4/e5: Mid-board positions for single-step tests
    ///          - e5/e4: En passant positions for capture tests
    ///          Adapted from existing PawnTest SetUp() pattern.
    void SetUp() override
    {
      // Initialize grid to standard position
      test_grid.initialize_standard_position();
      
      p_e2_white_pawn = std::make_unique<Chess::ChessPawn>(Chess::Color::WHITE, Chess::Position(4, 1));
      p_e7_black_pawn = std::make_unique<Chess::ChessPawn>(Chess::Color::BLACK, Chess::Position(4, 6));

      p_e4_white_pawn = std::make_unique<Chess::ChessPawn>(Chess::Color::WHITE, Chess::Position(4, 3));
      p_e5_black_pawn = std::make_unique<Chess::ChessPawn>(Chess::Color::BLACK, Chess::Position(4, 4));

      p_e5_white_pawn = std::make_unique<Chess::ChessPawn>(Chess::Color::WHITE, Chess::Position(4, 4));
      p_e4_black_pawn = std::make_unique<Chess::ChessPawn>(Chess::Color::BLACK, Chess::Position(4, 3));
    }

    /// @brief   Clean up test environment after each test.
    /// @details Automatically releases all ChessPawn instances using smart pointer reset.
    void TearDown() override
    {
      p_e2_white_pawn.reset();
      p_e7_black_pawn.reset();

      p_e4_white_pawn.reset();
      p_e5_black_pawn.reset();

      p_e5_white_pawn.reset();
      p_e4_black_pawn.reset();
    }
};

/// @test   Verify ChessPawn creation and basic properties.
/// @details Adapted from existing CreationState test pattern.
TEST_F(ChessPawnTest, CreationState)
{
  // Test white pawn creation
  EXPECT_EQ(p_e2_white_pawn->get_type(), Chess::PieceType::PAWN);
  EXPECT_EQ(p_e2_white_pawn->get_color(), Chess::Color::WHITE);
  EXPECT_EQ(p_e2_white_pawn->get_position(), Chess::Position(4, 1));
  EXPECT_EQ(p_e2_white_pawn->get_representation(), 'P');

  // Test black pawn creation
  EXPECT_EQ(p_e7_black_pawn->get_type(), Chess::PieceType::PAWN);
  EXPECT_EQ(p_e7_black_pawn->get_color(), Chess::Color::BLACK);
  EXPECT_EQ(p_e7_black_pawn->get_position(), Chess::Position(4, 6));
  EXPECT_EQ(p_e7_black_pawn->get_representation(), 'p');
}

/// @test   Verify single and double step forward movement from starting position.
/// @details Adapted from existing StartStepForward test pattern.
TEST_F(ChessPawnTest, StartStepForward)
{
  // Clear test vectors (adapted from existing pattern)
  moves = {};
  expected_moves = {};
  other_pieces = {};
  other_colors = {};

  // Test white pawn from e2 (starting position)
  p_e2_white_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // White pawn should be able to move to e3 and e4 from starting position
  expected_moves.push_back(Chess::Position(4, 2)); // e3
  expected_moves.push_back(Chess::Position(4, 3)); // e4
  
  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }

  // Test black pawn from e7 (starting position)
  moves = {};
  expected_moves = {};
  
  p_e7_black_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // Black pawn should be able to move to e6 and e5 from starting position
  expected_moves.push_back(Chess::Position(4, 5)); // e6
  expected_moves.push_back(Chess::Position(4, 4)); // e5
  
  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }
}

/// @test   Verify single step forward movement from non-starting position.
/// @details Adapted from existing NoDoubleStepFromNonStartPosition test pattern.
TEST_F(ChessPawnTest, SingleStepFromNonStartPosition)
{
  // Clear test vectors (adapted from existing pattern)
  moves = {};
  expected_moves = {};
  other_pieces = {};
  other_colors = {};

  // Test white pawn from e4 (non-starting position)
  p_e4_white_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // White pawn should only be able to move to e5 (single step)
  expected_moves.push_back(Chess::Position(4, 4)); // e5
  
  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }

  // Test black pawn from e5 (non-starting position)
  moves = {};
  expected_moves = {};
  
  p_e5_black_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // Black pawn should only be able to move to e4 (single step)
  expected_moves.push_back(Chess::Position(4, 3)); // e4
  
  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }
}

/// @test   Verify diagonal capture moves.
/// @details Adapted from existing DiagonalCapture test pattern.
TEST_F(ChessPawnTest, DiagonalCapture)
{
  // Clear test vectors (adapted from existing pattern)
  moves = {};
  expected_moves = {};
  
  // Set up pieces for capture test
  // White pawn at e4, black pieces at d5 and f5
  other_pieces = {
    Chess::Position(3, 4), // d5 - black pawn
    Chess::Position(5, 4)  // f5 - black pawn
  };
  other_colors = {
    Chess::Color::BLACK,
    Chess::Color::BLACK
  };

  p_e4_white_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // White pawn should be able to capture d5 and f5, plus move forward to e5
  expected_moves.push_back(Chess::Position(4, 4)); // e5 (forward)
  expected_moves.push_back(Chess::Position(3, 4)); // d5 (capture)
  expected_moves.push_back(Chess::Position(5, 4)); // f5 (capture)
  
  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }
}

/// @test   Verify pawns cannot capture own-colored pieces.
/// @details Adapted from existing CannotCaptureOwnPieces test pattern.
TEST_F(ChessPawnTest, CannotCaptureOwnPieces)
{
  // Clear test vectors (adapted from existing pattern)
  moves = {};
  expected_moves = {};
  
  // Set up pieces with own-colored pieces blocking diagonal captures
  // White pawn at e4, white pieces at d5 and f5
  other_pieces = {
    Chess::Position(3, 4), // d5 - white pawn (same color)
    Chess::Position(5, 4)  // f5 - white pawn (same color)
  };
  other_colors = {
    Chess::Color::WHITE,
    Chess::Color::WHITE
  };

  p_e4_white_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // White pawn should only be able to move forward to e5 (no captures)
  expected_moves.push_back(Chess::Position(4, 4)); // e5 (forward only)
  
  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }
}

/// @test   Verify en passant capture moves for white pawns.
/// @details Adapted from existing EnPassantCaptureWhite test pattern.
TEST_F(ChessPawnTest, EnPassantCaptureWhite)
{
  // Clear test vectors (adapted from existing pattern)
  moves = {};
  expected_moves = {};
  other_pieces = {};
  other_colors = {};

  // Set up en passant scenario: white pawn at e5, black pawn just moved from d7 to d5
  // We need to simulate this by setting up the grid state appropriately
  test_grid.flags.halfmove_clock = 0; // Indicates a pawn just moved
  
  // Place a black pawn at d5 (position that just moved two squares)
  other_pieces = {
    Chess::Position(3, 3)  // d5 - black pawn
  };
  other_colors = {
    Chess::Color::BLACK
  };

  p_e5_white_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // White pawn should be able to capture en passant at d6
  expected_moves.push_back(Chess::Position(4, 5)); // e6 (forward)
  expected_moves.push_back(Chess::Position(3, 5)); // d6 (en passant capture)
  
  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }
}

/// @test   Verify en passant capture moves for black pawns.
/// @details Adapted from existing EnPassantCaptureBlack test pattern.
TEST_F(ChessPawnTest, EnPassantCaptureBlack)
{
  // Clear test vectors (adapted from existing pattern)
  moves = {};
  expected_moves = {};
  other_pieces = {};
  other_colors = {};

  // Set up en passant scenario: black pawn at e4, white pawn just moved from e2 to e4
  test_grid.flags.halfmove_clock = 0; // Indicates a pawn just moved
  
  // Place a white pawn at e4 (position that just moved two squares)
  other_pieces = {
    Chess::Position(4, 3)  // e4 - white pawn
  };
  other_colors = {
    Chess::Color::WHITE
  };

  p_e4_black_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // Black pawn should be able to capture en passant at e3
  expected_moves.push_back(Chess::Position(4, 2)); // e3 (forward)
  expected_moves.push_back(Chess::Position(4, 2)); // e3 (en passant capture - same as forward in this case)
  
  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }
}

/// @test   Verify pawn movement is blocked by pieces in front.
/// @details Adapted from existing blocking patterns.
TEST_F(ChessPawnTest, BlockedForwardMovement)
{
  // Clear test vectors (adapted from existing pattern)
  moves = {};
  expected_moves = {};
  
  // Set up piece blocking forward movement
  // White pawn at e2, black pawn at e3
  other_pieces = {
    Chess::Position(4, 2)  // e3 - black pawn blocking
  };
  other_colors = {
    Chess::Color::BLACK
  };

  p_e2_white_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // White pawn should have no moves (blocked forward, no diagonal captures)
  expected_moves = {}; // No valid moves
  
  EXPECT_EQ(moves.size(), expected_moves.size());
}

/// @test   Verify pawn position setting works correctly.
/// @details Adapted from existing position management patterns.
TEST_F(ChessPawnTest, PositionManagement)
{
  // Test position setting
  Chess::Position new_pos(6, 2);
  p_e2_white_pawn->set_position(new_pos);
  
  EXPECT_EQ(p_e2_white_pawn->get_position(), new_pos);
  
  // Test movement from new position
  moves = {};
  expected_moves = {};
  other_pieces = {};
  other_colors = {};
  
  p_e2_white_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // From g2, should be able to move to g3 and g4 (still starting position)
  expected_moves.push_back(Chess::Position(6, 3)); // g3
  expected_moves.push_back(Chess::Position(6, 4)); // g4
  
  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }
}
