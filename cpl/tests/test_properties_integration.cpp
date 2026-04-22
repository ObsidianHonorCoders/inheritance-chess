/// @file      test_properties_integration.cpp
/// @brief     Unit tests for Properties integration in CPL pieces.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite for Properties integration with special moves:
///             - En passant capture functionality
///             - Castling rights and movement
///             - Game state tracking
///             - Halfmove clock functionality

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../include/chess_pawn.h"
#include "../include/chess_king.h"
#include "../../cci/include/grid.h"

/// @class   PropertiesIntegrationTest
/// @brief   Test fixture class for Properties integration tests.
/// @details Tests special moves that depend on game state Properties.
class PropertiesIntegrationTest : public ::testing::Test
{
  protected:
    /// @brief   Grid instance for testing.
    /// @details Uses CCI Grid structure with GameFlags for Properties integration.
    Chess::Grid test_grid;

    /// @brief   Available moves vector for test results.
    /// @details Populated by piece's available_moves() method during testing.
    Chess::PositionList moves = {};

    /// @brief   Expected moves vector for test validation.
    /// @details Contains the positions we expect the piece to be able to move to.
    Chess::PositionList expected_moves = {};

    /// @brief   Other pieces positions on the board for move validation.
    /// @details Represents positions of all other pieces during testing.
    Chess::PositionList other_pieces = {};

    /// @brief   Colors corresponding to other_pieces for capture validation.
    /// @details Used to determine which pieces can be captured.
    Chess::ColorList other_colors = {};

    /// @brief   Set up test environment before each test.
    /// @details Initializes grid and test vectors for Properties testing.
    void SetUp() override
    {
      // Initialize grid to standard position
      test_grid.initialize_standard_position();
      
      // Clear test vectors
      moves = {};
      expected_moves = {};
      other_pieces = {};
      other_colors = {};
    }

    /// @brief   Clean up test environment after each test.
    /// @details Automatically releases resources using smart pointer patterns.
    void TearDown() override
    {
      // Smart pointers will automatically clean up
    }
};

/// @test   Verify en passant capture integration with Properties.
/// @details Tests that en passant works correctly when halfmove_clock is 0.
TEST_F(PropertiesIntegrationTest, EnPassantPropertiesIntegration)
{
  // Set up en passant scenario: white pawn at e5, black pawn at d5
  auto white_pawn = std::make_unique<Chess::ChessPawn>(Chess::Color::WHITE, Chess::Position(4, 4));
  
  // Place black pawn at d5 (position that just moved two squares)
  other_pieces = {
    Chess::Position(3, 3)  // d5 - black pawn
  };
  other_colors = {
    Chess::Color::BLACK
  };

  // Set Properties to indicate en passant is available
  test_grid.flags.halfmove_clock = 0; // Indicates a pawn just moved
  
  white_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
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

/// @test   Verify en passant is not available when halfmove_clock > 0.
/// @details Tests that en passant is correctly disabled after one move.
TEST_F(PropertiesIntegrationTest, EnPassantExpiredWhenHalfmoveClockNonZero)
{
  // Set up same scenario as above but with halfmove_clock > 0
  auto white_pawn = std::make_unique<Chess::ChessPawn>(Chess::Color::WHITE, Chess::Position(4, 4));
  
  other_pieces = {
    Chess::Position(3, 3)  // d5 - black pawn
  };
  other_colors = {
    Chess::Color::BLACK
  };

  // Set Properties to indicate en passant is NOT available
  test_grid.flags.halfmove_clock = 1; // More than 0 moves since last pawn move
  
  white_pawn->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // White pawn should only be able to move forward (no en passant)
  expected_moves.push_back(Chess::Position(4, 5)); // e6 (forward only)
  
  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }
}

/// @test   Verify kingside castling integration with Properties.
/// @details Tests that castling works when castling rights are available.
TEST_F(PropertiesIntegrationTest, KingsideCastlingPropertiesIntegration)
{
  // Set up white king at starting position
  auto white_king = std::make_unique<Chess::ChessKing>(Chess::Color::WHITE, Chess::Position(4, 0));
  
  // Ensure castling rights are available
  test_grid.flags.white_can_castle_kingside = true;
  test_grid.flags.white_can_castle_queenside = true;
  
  // Empty board (no other pieces)
  other_pieces = {};
  other_colors = {};
  
  white_king->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // King should have normal moves plus castling move
  expected_moves.push_back(Chess::Position(3, 0)); // d1
  expected_moves.push_back(Chess::Position(5, 0)); // f1
  expected_moves.push_back(Chess::Position(4, 1)); // e2
  expected_moves.push_back(Chess::Position(3, 1)); // d2
  expected_moves.push_back(Chess::Position(5, 1)); // f2
  expected_moves.push_back(Chess::Position(6, 0)); // g1 (kingside castling)
  
  EXPECT_GE(moves.size(), expected_moves.size()); // At least these moves should be available
  
  // Check that castling move is included
  bool found_castling = false;
  for (const auto& move : moves)
  {
    if (move.file == 6 && move.rank == 0) // g1
    {
      found_castling = true;
      break;
    }
  }
  EXPECT_TRUE(found_castling);
}

/// @test   Verify castling is disabled when castling rights are lost.
/// @details Tests that castling is correctly disabled when king moves.
TEST_F(PropertiesIntegrationTest, CastlingDisabledWhenRightsLost)
{
  // Set up white king at starting position
  auto white_king = std::make_unique<Chess::ChessKing>(Chess::Color::WHITE, Chess::Position(4, 0));
  
  // Disable castling rights
  test_grid.flags.white_can_castle_kingside = false;
  test_grid.flags.white_can_castle_queenside = false;
  
  // Empty board (no other pieces)
  other_pieces = {};
  other_colors = {};
  
  white_king->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // King should have normal moves but NO castling
  expected_moves.push_back(Chess::Position(3, 0)); // d1
  expected_moves.push_back(Chess::Position(5, 0)); // f1
  expected_moves.push_back(Chess::Position(4, 1)); // e2
  expected_moves.push_back(Chess::Position(3, 1)); // d2
  expected_moves.push_back(Chess::Position(5, 1)); // f2
  
  EXPECT_EQ(moves.size(), expected_moves.size());
  for (size_t i = 0; i < expected_moves.size(); i++)
  {
    EXPECT_EQ(moves[i].file, expected_moves[i].file);
    EXPECT_EQ(moves[i].rank, expected_moves[i].rank);
  }
}

/// @test   Verify black king castling integration with Properties.
/// @details Tests that black castling works correctly with black-specific Properties.
TEST_F(PropertiesIntegrationTest, BlackKingCastlingPropertiesIntegration)
{
  // Set up black king at starting position
  auto black_king = std::make_unique<Chess::ChessKing>(Chess::Color::BLACK, Chess::Position(4, 7));
  
  // Ensure black castling rights are available
  test_grid.flags.black_can_castle_kingside = true;
  test_grid.flags.black_can_castle_queenside = true;
  
  // Empty board (no other pieces)
  other_pieces = {};
  other_colors = {};
  
  black_king->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // Check that castling move is included for black king
  bool found_castling = false;
  for (const auto& move : moves)
  {
    if (move.file == 6 && move.rank == 7) // g8
    {
      found_castling = true;
      break;
    }
  }
  EXPECT_TRUE(found_castling);
}

/// @test   Verify castling is blocked by pieces in between.
/// @details Tests that castling is correctly blocked when path is not clear.
TEST_F(PropertiesIntegrationTest, CastlingBlockedByPieces)
{
  // Set up white king at starting position
  auto white_king = std::make_unique<Chess::ChessKing>(Chess::Color::WHITE, Chess::Position(4, 0));
  
  // Ensure castling rights are available
  test_grid.flags.white_can_castle_kingside = true;
  test_grid.flags.white_can_castle_queenside = true;
  
  // Place a piece between king and rook (blocking castling)
  other_pieces = {
    Chess::Position(5, 0),  // f1 - blocks kingside castling
    Chess::Position(3, 0)   // d1 - blocks queenside castling
  };
  other_colors = {
    Chess::Color::WHITE,
    Chess::Color::WHITE
  };
  
  white_king->available_moves(moves, other_pieces, other_colors, test_grid);
  
  // King should have normal moves but NO castling
  expected_moves.push_back(Chess::Position(4, 1)); // e2
  expected_moves.push_back(Chess::Position(3, 1)); // d2
  expected_moves.push_back(Chess::Position(5, 1)); // f2
  
  // Should not include castling moves
  for (const auto& move : moves)
  {
    // Check no castling moves are present
    EXPECT_NE(move.file, 6); // No g1 (kingside castling)
    EXPECT_NE(move.file, 2); // No c1 (queenside castling)
  }
}

/// @test   Verify Properties initialization works correctly.
/// @details Tests that GameFlags are properly initialized for standard position.
TEST_F(PropertiesIntegrationTest, PropertiesInitialization)
{
  // Test that standard position has correct Properties
  EXPECT_EQ(test_grid.flags.white_can_castle_kingside, true);
  EXPECT_EQ(test_grid.flags.white_can_castle_queenside, true);
  EXPECT_EQ(test_grid.flags.black_can_castle_kingside, true);
  EXPECT_EQ(test_grid.flags.black_can_castle_queenside, true);
  EXPECT_EQ(test_grid.flags.halfmove_clock, 0);
  EXPECT_EQ(test_grid.flags.fullmove_number, 1);
}

/// @test   Verify turn management integrates with Properties.
/// @details Tests that turn switching works with Properties structure.
TEST_F(PropertiesIntegrationTest, TurnManagementIntegration)
{
  // Test initial turn
  EXPECT_EQ(test_grid.current_turn, Chess::Color::WHITE);
  
  // Switch turn
  test_grid.switch_turn();
  EXPECT_EQ(test_grid.current_turn, Chess::Color::BLACK);
  
  // Switch back
  test_grid.switch_turn();
  EXPECT_EQ(test_grid.current_turn, Chess::Color::WHITE);
}
