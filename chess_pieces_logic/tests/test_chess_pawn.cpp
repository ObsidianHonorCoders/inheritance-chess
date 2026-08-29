/// @file      test_chess_pawn.cpp
/// @namespace Chess
/// @brief     Unit tests for ChessPawn class using Google Test framework.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite covering ChessPawn movement logic:
///             - Construction and initial property values
///             - Single and double-step forward movement from starting rank
///             - Single-step restriction from non-starting ranks
///             - Diagonal capture and own-piece capture prevention
///             - En passant capture for both white and black
///             - Blocked forward movement and position mutation
/// @note      Each test pawn is heap-allocated via std::unique_ptr and released
///            explicitly in TearDown to verify reset semantics.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../include/chess_pawn.h"
#include "../../chess_common_interface/include/grid.h"

/// @class   ChessPawnTest
/// @brief   Test fixture class for ChessPawn unit tests.
/// @details Pre-constructs six ChessPawn instances at starting positions (e2/e7),
///          mid-board positions (e4/e5), and en passant positions (e5/e4) so
///          each test can exercise move logic without repeated setup code.
class ChessPawnTest : public ::testing::Test
{
  protected:
    /// @brief   Grid instance for testing.
    /// @details Uses CCI Grid structure for board state.
    Chess::Grid test_grid;

    /// @brief   Available moves vector for test results.
    /// @details Populated by a pawn’s available_moves() call during testing.
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
    /// @details Constructs six ChessPawn instances at predefined positions,
    ///          initializes test_grid to the standard opening position, and
    ///          resets the auxiliary move vectors to empty.
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
/// @details Checks get_type(), get_color(), get_position(), and get_representation()
///          for both a white pawn at e2 and a black pawn at e7.
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
/// @details From the starting rank, available_moves() should return exactly two
///          positions: one square and two squares forward. Verified for both
///          the white pawn at e2 and the black pawn at e7.
TEST_F(ChessPawnTest, StartStepForward)
{
  moves          = {};
  expected_moves = {};
  other_pieces   = {};
  other_colors   = {};

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
  moves          = {};
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
/// @details From a non-starting rank, available_moves() should return exactly
///          one position: one square forward. Verified for white (e4) and black (e5).
TEST_F(ChessPawnTest, SingleStepFromNonStartPosition)
{
  moves          = {};
  expected_moves = {};
  other_pieces   = {};
  other_colors   = {};

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
  moves          = {};
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
/// @details With opponent pawns on d5 and f5, the white pawn at e4 should
///          return three moves: one forward (e5) plus two diagonal captures.
TEST_F(ChessPawnTest, DiagonalCapture)
{
  moves          = {};
  expected_moves = {};

  // Set up pieces for capture test
  // White pawn at e4, black pieces at d5 and f5
  other_pieces = {
      Chess::Position(3, 4), // d5 - black pawn
      Chess::Position(5, 4)  // f5 - black pawn
  };
  other_colors = {Chess::Color::BLACK, Chess::Color::BLACK};

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
/// @details With same-color pawns on d5 and f5, the white pawn at e4 should
///          return only the forward move (e5); no captures.
TEST_F(ChessPawnTest, CannotCaptureOwnPieces)
{
  moves          = {};
  expected_moves = {};

  // Set up pieces with own-colored pieces blocking diagonal captures
  // White pawn at e4, white pieces at d5 and f5
  other_pieces = {
      Chess::Position(3, 4), // d5 - white pawn (same color)
      Chess::Position(5, 4)  // f5 - white pawn (same color)
  };
  other_colors = {Chess::Color::WHITE, Chess::Color::WHITE};

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
/// @details With halfmove_clock == 0 and a black pawn at d5 flagged via
///          set_piece(), the white pawn at e5 should include d6 as an
///          en passant capture alongside the regular e6 forward move.
TEST_F(ChessPawnTest, EnPassantCaptureWhite)
{
  moves          = {};
  expected_moves = {};
  other_pieces   = {};
  other_colors   = {};

  // Set up en passant scenario: white pawn at e5, black pawn just moved from d7 to d5
  // We need to simulate this by setting up the grid state appropriately
  test_grid.flags.halfmove_clock = 0; // Indicates a pawn just moved

  // Place a black pawn at d5 on the grid (position that just moved two squares)
  Chess::PieceProperties black_pawn;
  black_pawn.type     = Chess::PieceType::PAWN;
  black_pawn.color    = Chess::Color::BLACK;
  black_pawn.position = Chess::Position(3, 4); // d5 - black pawn (rank index 4 for rank 5)
  test_grid.set_piece(Chess::Position(3, 4), black_pawn);

  other_pieces = {
      Chess::Position(3, 4) // d5 - black pawn
  };
  other_colors = {Chess::Color::BLACK};

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
/// @details With halfmove_clock == 0 and a white pawn at d4 marked
///          en_passant_vulnerable, a black pawn at e4 should include d3
///          as an en passant capture alongside the regular e3 forward move.
TEST_F(ChessPawnTest, EnPassantCaptureBlack)
{
  moves          = {};
  expected_moves = {};
  other_pieces   = {};
  other_colors   = {};

  // Set up en passant scenario: black pawn at e4, white pawn just moved to d4
  test_grid.flags.halfmove_clock = 0; // Indicates a pawn just moved

    // Place a white pawn at d4 on the grid (position that just moved two squares)
  Chess::PieceProperties white_pawn;
  white_pawn.type     = Chess::PieceType::PAWN;
  white_pawn.color    = Chess::Color::WHITE;
    white_pawn.position = Chess::Position(3, 3); // d4 - white pawn (rank index 3 for rank 4)
  white_pawn.en_passant_vulnerable = true; // This pawn is vulnerable to en passant
    test_grid.set_piece(Chess::Position(3, 3), white_pawn);

  other_pieces = {
      Chess::Position(3, 3) // d4 - white pawn
  };
  other_colors = {Chess::Color::WHITE};

    auto p_e4_black_temp = std::make_unique<Chess::ChessPawn>(Chess::Color::BLACK, Chess::Position(4, 3));
    p_e4_black_temp->available_moves(moves, other_pieces, other_colors, test_grid);

    // Black pawn should be able to move to e3 and capture en passant on d3
    expected_moves.push_back(Chess::Position(4, 2)); // e3 (forward)
    expected_moves.push_back(Chess::Position(3, 2)); // d3 (en passant capture)

  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }
}

/// @test   Verify pawn movement is blocked by pieces in front.
/// @details With a black pawn on e3 blocking the forward square, the white
///          pawn at e2 should return no valid moves.
TEST_F(ChessPawnTest, BlockedForwardMovement)
{
  moves          = {};
  expected_moves = {};

  // Set up piece blocking forward movement
  // White pawn at e2, black pawn at e3
  other_pieces = {
      Chess::Position(4, 2) // e3 - black pawn blocking
  };
  other_colors = {Chess::Color::BLACK};

  p_e2_white_pawn->available_moves(moves, other_pieces, other_colors, test_grid);

  // White pawn should have no moves (blocked forward, no diagonal captures)
  expected_moves = {}; // No valid moves

  EXPECT_EQ(moves.size(), expected_moves.size());
}

/// @test   Verify pawn position setting works correctly.
/// @details Calls set_position() to relocate a pawn to g3 (a non-starting rank)
///          and confirms get_position() reflects the new square and
///          available_moves() returns only the single forward move.
TEST_F(ChessPawnTest, PositionManagement)
{
  // Test position setting
  Chess::Position new_pos(6, 2);
  p_e2_white_pawn->set_position(new_pos);

  EXPECT_EQ(p_e2_white_pawn->get_position(), new_pos);

  // Test movement from new position
  moves          = {};
  expected_moves = {};
  other_pieces   = {};
  other_colors   = {};

  p_e2_white_pawn->available_moves(moves, other_pieces, other_colors, test_grid);

  // From g3 (rank index 2), should only be able to move one square to g4
  expected_moves.push_back(Chess::Position(6, 3)); // g3

  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }
}
