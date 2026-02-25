/// @file      test_pawns.cpp
/// @brief     Unit tests for the Pawn class using Google Test framework.
/// @author    Calileus
/// @date      2026-02-24
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite for Pawn class functionality including:
///             - Pawn creation and initialization
///             - Single and double step forward movement
///             - Diagonal capture moves
///             - En passant capture moves
/// @note      Uses std::unique_ptr for automatic memory management
///            following modern C++ RAII principles.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "pawns.hpp"

/// @class   PawnTest
/// @brief   Test fixture class for Pawn unit tests.
/// @details Provides setup and teardown methods for test environment.
///          Uses smart pointers for automatic memory management.
///          Pieces are not in a board.
class PawnTest : public ::testing::Test
{
  protected:
    /// @brief   Default board properties for testing.
    /// @details Contains default values for castling rights, last move info,
    ///          and pawn move counters used across all tests.
    Properties props = {};

    /// @brief   Available moves vector for test results.
    /// @details Populated by pawn's available_moves() method during testing.
    Piece::PositionList moves = {};

    /// @brief   Expected moves vector for test validation.
    /// @details Contains the positions we expect the pawn to be able to move to.
    Piece::PositionList expected_moves = {};

    /// @brief   Other pieces positions on the board for move validation.
    /// @details Represents positions of all other pieces during testing.
    Piece::PositionList other_pieces = {};

    /// @brief   Colors corresponding to other_pieces for capture validation.
    /// @details Used to determine which pieces can be captured.
    Piece::ColorList other_colors = {};

    /// @brief   Smart pointer to a white Pawn instance under test.
    /// @details Uses std::unique_ptr for automatic memory management.
    std::unique_ptr<Pawn> p_e2_white_pawn;

    /// @brief   Smart pointer to a black Pawn instance under test.
    /// @details Uses std::unique_ptr for automatic memory management.
    std::unique_ptr<Pawn> p_e7_black_pawn;

    /// @brief   Smart pointer to a white Pawn at e4 (mid-board position).
    /// @details Used for testing single-step movement from non-starting positions.
    std::unique_ptr<Pawn> p_e4_white_pawn;

    /// @brief   Smart pointer to a black Pawn at e5 (mid-board position).
    /// @details Used for testing single-step movement from non-starting positions.
    std::unique_ptr<Pawn> p_e5_black_pawn;

    /// @brief   Smart pointer to a white Pawn at e5 (en passant position).
    /// @details Used for testing en passant capture scenarios.
    std::unique_ptr<Pawn> p_e5_white_pawn;

    /// @brief   Smart pointer to a black Pawn at e4 (en passant position).
    /// @details Used for testing en passant capture scenarios.
    std::unique_ptr<Pawn> p_e4_black_pawn;

    /// @brief   Set up test environment before each test.
    /// @details Creates new Pawn instances at various positions for comprehensive testing:
    ///          - e2/e7: Starting positions for forward movement tests
    ///          - e4/e5: Mid-board positions for single-step tests
    ///          - e5/e4: En passant positions for capture tests
    void SetUp() override
    {
      p_e2_white_pawn = std::make_unique<Pawn>('e', '2', Piece::Color::WHITE);
      p_e7_black_pawn = std::make_unique<Pawn>('e', '7', Piece::Color::BLACK);

      p_e4_white_pawn = std::make_unique<Pawn>('e', '4', Piece::Color::WHITE);
      p_e5_black_pawn = std::make_unique<Pawn>('e', '5', Piece::Color::BLACK);

      p_e5_white_pawn = std::make_unique<Pawn>('e', '5', Piece::Color::WHITE);
      p_e4_black_pawn = std::make_unique<Pawn>('e', '4', Piece::Color::BLACK);
    }

    /// @brief   Clean up test environment after each test.
    /// @details Automatically releases all Pawn instances using smart pointer reset.
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

/// @brief   Test pawn creation and smart pointer initialization.
/// @details Verifies that all Pawn instances are properly created during SetUp
///          and the smart pointers are not null. Tests all pawn positions used
///          throughout the test suite.
TEST_F(PawnTest, CreationState)
{
  // Expect valid smart pointers from SetUp
  EXPECT_NE(p_e2_white_pawn.get(), nullptr);
  EXPECT_NE(p_e7_black_pawn.get(), nullptr);
  EXPECT_NE(p_e4_white_pawn.get(), nullptr);
  EXPECT_NE(p_e5_black_pawn.get(), nullptr);
}

/// @brief   Test forward movement for pawns at starting positions.
/// @details Verifies that white and black pawns can move one or two squares
///          forward from their starting positions (e2 for white, e7 for black)
///          when the target squares are empty.
TEST_F(PawnTest, StartStepForward)
{
  props = default_properties;
  other_pieces.clear();
  other_colors.clear();

  // White pawn at e2 should be able to move to e3 and e4
  expected_moves.clear();
  expected_moves.push_back({'e', '3'});
  expected_moves.push_back({'e', '4'});
  p_e2_white_pawn->available_moves(moves, other_pieces, other_colors, props);

  EXPECT_EQ(moves.size(), 2);
  for (int i = 0; i < moves.size(); i++)
  {
    EXPECT_TRUE(expected_moves[i].file == moves[i].file);
    EXPECT_TRUE(expected_moves[i].rank == moves[i].rank);
  }

  // Black pawn at e7 should be able to move to e6 and e5
  expected_moves.clear();
  expected_moves.push_back({'e', '6'});
  expected_moves.push_back({'e', '5'});
  p_e7_black_pawn->available_moves(moves, other_pieces, other_colors, props);

  EXPECT_EQ(moves.size(), 2);
  for (int i = 0; i < moves.size(); i++)
  {
    EXPECT_TRUE(expected_moves[i].file == moves[i].file);
    EXPECT_TRUE(expected_moves[i].rank == moves[i].rank);
  }
}

/// @brief   Test that double step is not available from non-starting positions.
/// @details Verifies that pawns cannot move two squares forward
///          when not on their starting rank.
TEST_F(PawnTest, NoDoubleStepFromNonStartPosition)
{
  props = default_properties;
  other_pieces.clear();
  other_colors.clear();

  // White pawn at e4 should only be able to move to e5
  expected_moves.clear();
  expected_moves.push_back({'e', '5'});
  p_e4_white_pawn->available_moves(moves, other_pieces, other_colors, props);

  EXPECT_EQ(moves.size(), 1);
  EXPECT_TRUE(expected_moves[0].file == moves[0].file);
  EXPECT_TRUE(expected_moves[0].rank == moves[0].rank);

  // Black pawn at e5 should only be able to move to e4
  expected_moves.clear();
  expected_moves.push_back({'e', '4'});
  p_e5_black_pawn->available_moves(moves, other_pieces, other_colors, props);

  EXPECT_EQ(moves.size(), 1);
  EXPECT_TRUE(expected_moves[0].file == moves[0].file);
  EXPECT_TRUE(expected_moves[0].rank == moves[0].rank);
}

/// @brief   Test diagonal capture moves for pawns.
/// @details Verifies that pawns can capture opponent pieces diagonally while
///          also maintaining their normal forward movement capabilities.
///          Tests both white and black pawns with opponent pieces on diagonal squares.
TEST_F(PawnTest, DiagonalCapture)
{
  props = default_properties;

  // White pawn at e2 with black pieces at d3 and f3
  // Should be able to capture d3 and f3, plus move forward to e3 and e4
  other_pieces = {{'d', '3'}, {'f', '3'}}; // Only diagonal pieces
  other_colors = {Piece::Color::BLACK, Piece::Color::BLACK};
  expected_moves.push_back({'e', '3'});
  expected_moves.push_back({'e', '4'});
  expected_moves.push_back({'d', '3'});
  expected_moves.push_back({'f', '3'});
  p_e2_white_pawn->available_moves(moves, other_pieces, other_colors, props);

  EXPECT_EQ(moves.size(), 4);
  for (int i = 0; i < moves.size(); i++)
  {
    EXPECT_TRUE(expected_moves[i].file == moves[i].file);
    EXPECT_TRUE(expected_moves[i].rank == moves[i].rank);
  }

  // Black pawn at e7 with white pieces at d6 and f6
  // Should be able to capture d6 and f6, plus move forward to e6 and e5
  other_pieces = {{'d', '6'}, {'f', '6'}}; // Only diagonal pieces
  other_colors = {Piece::Color::WHITE, Piece::Color::WHITE};
  expected_moves.clear();
  expected_moves.push_back({'e', '6'});
  expected_moves.push_back({'e', '5'});
  expected_moves.push_back({'d', '6'});
  expected_moves.push_back({'f', '6'});
  p_e7_black_pawn->available_moves(moves, other_pieces, other_colors, props);

  EXPECT_EQ(moves.size(), 4);
  for (int i = 0; i < moves.size(); i++)
  {
    EXPECT_TRUE(expected_moves[i].file == moves[i].file);
    EXPECT_TRUE(expected_moves[i].rank == moves[i].rank);
  }
}

/// @brief   Test that pawns cannot capture own pieces.
/// @details Verifies that pawns cannot capture pieces of the same color.
TEST_F(PawnTest, CannotCaptureOwnPieces)
{
  props = default_properties;

  other_pieces = {{'d', '3'}, {'f', '3'}, {'e', '4'}};
  other_colors = {Piece::Color::WHITE, Piece::Color::WHITE, Piece::Color::WHITE};

  // White pawn at e2 with white pieces blocking d3, f3, and e4
  // Should only be able to move forward to e3 (captures blocked, e4 blocked)
  expected_moves.clear();
  expected_moves.push_back({'e', '3'});
  p_e2_white_pawn->available_moves(moves, other_pieces, other_colors, props);

  EXPECT_EQ(moves.size(), 1);
  EXPECT_TRUE(expected_moves[0].file == moves[0].file);
  EXPECT_TRUE(expected_moves[0].rank == moves[0].rank);

  other_pieces = {{'d', '6'}, {'f', '6'}, {'e', '5'}};
  other_colors = {Piece::Color::BLACK, Piece::Color::BLACK, Piece::Color::BLACK};

  // Black pawn at e7 with black pieces blocking d6, f6, and e5
  // Should only be able to move forward to e6 (captures blocked, e5 blocked)
  expected_moves.clear();
  expected_moves.push_back({'e', '6'});
  p_e7_black_pawn->available_moves(moves, other_pieces, other_colors, props);

  EXPECT_EQ(moves.size(), 1);
  EXPECT_TRUE(expected_moves[0].file == moves[0].file);
  EXPECT_TRUE(expected_moves[0].rank == moves[0].rank);
}

/// @brief   Test en passant capture for both white and black pawns.
/// @details Verifies that pawns can capture en passant when opponent pawns
///          move two squares forward from their starting positions. Tests both
///          white capturing black's en passant and black capturing white's en passant.
TEST_F(PawnTest, EnPassantCapture)
{
  // Set up properties for white en passant: black pawn just moved from d7 to d5
  props                       = default_properties;
  props.last_move_start       = {'d', '7'};
  props.last_move_end         = {'d', '5'};
  props.turns_since_pawn_move = 0;

  other_pieces = {{'d', '5'}}; // Black pawn at d5
  other_colors = {Piece::Color::BLACK};

  // White pawn at e5 should be able to capture en passant at d6
  expected_moves.clear();
  expected_moves.push_back({'e', '6'});
  expected_moves.push_back({'d', '6'});
  p_e5_white_pawn->available_moves(moves, other_pieces, other_colors, props);

  EXPECT_EQ(moves.size(), 2);
  for (int i = 0; i < moves.size(); i++)
  {
    EXPECT_TRUE(expected_moves[i].file == moves[i].file);
    EXPECT_TRUE(expected_moves[i].rank == moves[i].rank);
  }

  // Set up properties for black en passant: white pawn just moved from d2 to d4
  props                       = default_properties;
  props.last_move_start       = {'d', '2'};
  props.last_move_end         = {'d', '4'};
  props.turns_since_pawn_move = 0;

  other_pieces = {{'d', '4'}}; // White pawn at d4
  other_colors = {Piece::Color::WHITE};

  // Black pawn at e4 should be able to capture en passant at d3
  expected_moves.clear();
  expected_moves.push_back({'e', '3'});
  expected_moves.push_back({'d', '3'});
  p_e4_black_pawn->available_moves(moves, other_pieces, other_colors, props);

  EXPECT_EQ(moves.size(), 2);
  for (int i = 0; i < moves.size(); i++)
  {
    EXPECT_TRUE(expected_moves[i].file == moves[i].file);
    EXPECT_TRUE(expected_moves[i].rank == moves[i].rank);
  }
}

/// @brief   Test that en passant is not available after one turn.
/// @details Verifies that en passant capture opportunity expires
///          after one turn (when turns_since_pawn_move > 0).
TEST_F(PawnTest, EnPassantExpiresAfterOneTurn)
{
  // Set up properties where en passant should not be available (turns_since_pawn_move > 0)
  props                       = default_properties;
  props.last_move_start       = {'d', '7'};
  props.last_move_end         = {'d', '5'};
  props.turns_since_pawn_move = 1; // More than 0, so en passant not available

  other_pieces = {{'d', '5'}}; // Black pawn at d5
  other_colors = {Piece::Color::BLACK};

  // White pawn at e5 should only have forward move, no en passant
  expected_moves.clear();
  expected_moves.push_back({'e', '6'});
  p_e5_white_pawn->available_moves(moves, other_pieces, other_colors, props);

  EXPECT_EQ(moves.size(), 1);
  for (int i = 0; i < moves.size(); i++)
  {
    EXPECT_TRUE(expected_moves[i].file == moves[i].file);
    EXPECT_TRUE(expected_moves[i].rank == moves[i].rank);
  }
}
