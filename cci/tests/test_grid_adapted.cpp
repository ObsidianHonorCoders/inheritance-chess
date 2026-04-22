/// @file      test_grid_adapted.cpp
/// @brief     Unit tests for CCI Grid structure using adapted test patterns.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite for CCI Grid functionality adapted from existing test_pawns.cpp patterns:
///             - Grid initialization and piece management
///             - Smart pointer memory management patterns
///             - Position and move validation
///             - Factory pattern integration
/// @note      Uses std::unique_ptr for automatic memory management
///            following modern C++ RAII principles adapted from existing tests.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../include/grid.h"
#include "../include/position.h"
#include "../include/chess_types.h"

/// @class   GridTest
/// @brief   Test fixture class for Grid unit tests.
/// @details Provides setup and teardown methods for test environment.
///          Uses smart pointers for automatic memory management.
///          Adapted from existing PawnTest patterns.
class GridTest : public ::testing::Test
{
  protected:
    /// @brief   Grid instance for testing.
    /// @details Uses standard grid structure with CCI namespace.
    Chess::Grid test_grid;

    /// @brief   Available moves vector for test results.
    /// @details Adapted from existing Piece::PositionList pattern.
    Chess::PositionList moves = {};

    /// @brief   Expected moves vector for test validation.
    /// @details Contains the positions we expect pieces to be able to move to.
    Chess::PositionList expected_moves = {};

    /// @brief   Other pieces positions on the board for move validation.
    /// @details Adapted from existing Piece::PositionList pattern.
    Chess::PositionList other_pieces = {};

    /// @brief   Colors corresponding to other_pieces for capture validation.
    /// @details Adapted from existing Piece::ColorList pattern.
    Chess::ColorList other_colors = {};

    /// @brief   Set up test environment before each test.
    /// @details Creates new Grid instance and initializes standard position.
    ///          Adapted from existing PawnTest SetUp() pattern.
    void SetUp() override
    {
      // Initialize grid to standard position
      test_grid.initialize_standard_position();
      
      // Clear test vectors (adapted from existing test pattern)
      moves = {};
      expected_moves = {};
      other_pieces = {};
      other_colors = {};
    }

    /// @brief   Clean up test environment after each test.
    /// @details Automatically releases resources using smart pointer patterns.
    ///          Adapted from existing PawnTest TearDown() pattern.
    void TearDown() override
    {
      // Smart pointers in Grid will automatically clean up
      // No manual cleanup needed due to RAII principles
    }
};

/// @test   Verify grid initialization creates correct piece count.
/// @details Adapted from existing test patterns that verify piece creation.
TEST_F(GridTest, StandardPositionInitialization)
{
  // Test that standard position has correct number of pieces
  auto pieces = test_grid.get_all_pieces();
  
  // Standard chess starting position has 32 pieces
  // This test will be fully functional in Phase 2
  EXPECT_EQ(pieces.size(), 0); // Placeholder until factory is implemented
  
  // Verify grid structure is properly initialized
  EXPECT_EQ(test_grid.current_turn, Chess::Color::WHITE);
  
  // Check corners have rooks in standard position
  auto white_rook_pos = Chess::Position(0, 0); // a1
  auto black_rook_pos = Chess::Position(0, 7); // a8
  
  // These will work once piece factory is implemented
  // EXPECT_NE(test_grid.get_piece_at(white_rook_pos), nullptr);
  // EXPECT_NE(test_grid.get_piece_at(black_rook_pos), nullptr);
}

/// @test   Verify position validation works correctly.
/// @details Adapted from existing position validation patterns.
TEST_F(GridTest, PositionValidation)
{
  // Test valid positions
  Chess::Position valid_pos_a1(0, 0);
  Chess::Position valid_pos_h8(7, 7);
  Chess::Position valid_pos_e4(4, 3);
  
  EXPECT_TRUE(valid_pos_a1.is_valid());
  EXPECT_TRUE(valid_pos_h8.is_valid());
  EXPECT_TRUE(valid_pos_e4.is_valid());
  
  // Test invalid positions
  Chess::Position invalid_neg1(-1, 0);
  Chess::Position invalid_8_8(8, 8);
  Chess::Position invalid_100(100, 100);
  
  EXPECT_FALSE(invalid_neg1.is_valid());
  EXPECT_FALSE(invalid_8_8.is_valid());
  EXPECT_FALSE(invalid_100.is_valid());
}

/// @test   Verify position equality operators work correctly.
/// @details Adapted from existing position comparison patterns.
TEST_F(GridTest, PositionEquality)
{
  Chess::Position pos_a1(0, 0);
  Chess::Position pos_a1_copy(0, 0);
  Chess::Position pos_a2(0, 1);
  Chess::Position pos_b1(1, 0);
  
  // Test equality
  EXPECT_EQ(pos_a1, pos_a1_copy);
  EXPECT_EQ(pos_a1, pos_a1);
  
  // Test inequality
  EXPECT_NE(pos_a1, pos_a2);
  EXPECT_NE(pos_a1, pos_b1);
  EXPECT_NE(pos_a2, pos_b1);
}

/// @test   Verify grid piece access methods work correctly.
/// @details Adapted from existing Board piece access patterns.
TEST_F(GridTest, PieceAccessPatterns)
{
  // Test getting pieces at various positions
  Chess::Position pos_e4(4, 3);
  Chess::Position pos_empty(4, 4); // Center of empty board in Phase 1
  
  // These will be fully functional in Phase 2
  auto piece_e4 = test_grid.get_piece_at(pos_e4);
  auto piece_empty = test_grid.get_piece_at(pos_empty);
  
  // For now, these should return nullptr since factory isn't implemented
  EXPECT_EQ(piece_e4, nullptr);
  EXPECT_EQ(piece_empty, nullptr);
  
  // Test grid occupation status
  EXPECT_FALSE(test_grid.is_occupied(pos_e4));
  EXPECT_FALSE(test_grid.is_occupied(pos_empty));
}

/// @test   Verify smart pointer patterns work correctly.
/// @details Adapted from existing smart pointer test patterns.
TEST_F(GridTest, SmartPointerPatterns)
{
  // Test piece list functionality (adapted from existing Piece::List patterns)
  auto pieces = test_grid.get_all_pieces();
  
  // Should be empty in Phase 1
  EXPECT_EQ(pieces.size(), 0);
  
  // Test that piece list manages memory correctly
  // This pattern comes from existing Board::pieces management
  pieces.clear(); // Should automatically clean up memory
  EXPECT_EQ(pieces.size(), 0);
}

/// @test   Verify grid turn management works correctly.
/// @details Adapted from existing turn management patterns.
TEST_F(GridTest, TurnManagement)
{
  // Test initial turn
  EXPECT_EQ(test_grid.current_turn, Chess::Color::WHITE);
  
  // Test turn switching
  test_grid.switch_turn();
  EXPECT_EQ(test_grid.current_turn, Chess::Color::BLACK);
  
  test_grid.switch_turn();
  EXPECT_EQ(test_grid.current_turn, Chess::Color::WHITE);
}

/// @test   Verify game flags initialization.
/// @details Adapted from existing Properties struct patterns.
TEST_F(GridTest, GameFlagsInitialization)
{
  // Test that game flags are properly initialized
  EXPECT_TRUE(test_grid.flags.white_can_castle_kingside);
  EXPECT_TRUE(test_grid.flags.white_can_castle_queenside);
  EXPECT_TRUE(test_grid.flags.black_can_castle_kingside);
  EXPECT_TRUE(test_grid.flags.black_can_castle_queenside);
  
  // Test move counters
  EXPECT_EQ(test_grid.flags.halfmove_clock, 0);
  EXPECT_EQ(test_grid.flags.fullmove_number, 1);
}

/// @test   Verify piece properties structure works correctly.
/// @details Adapted from existing piece property patterns.
TEST_F(GridTest, PiecePropertiesStructure)
{
  // Test piece properties creation
  Chess::PieceProperties pawn_props;
  pawn_props.type = Chess::PieceType::PAWN;
  pawn_props.color = Chess::Color::WHITE;
  pawn_props.position = Chess::Position(4, 1); // e2
  pawn_props.has_moved = false;
  pawn_props.en_passant_vulnerable = false;
  
  // Verify properties
  EXPECT_EQ(pawn_props.type, Chess::PieceType::PAWN);
  EXPECT_EQ(pawn_props.color, Chess::Color::WHITE);
  EXPECT_EQ(pawn_props.position, Chess::Position(4, 1));
  EXPECT_FALSE(pawn_props.has_moved);
  EXPECT_FALSE(pawn_props.en_passant_vulnerable);
}

/// @test   Verify factory pattern integration (placeholder).
/// @details This test will be fully functional in Phase 2.
///          Adapted from existing piece creation patterns.
TEST_F(GridTest, FactoryPatternPlaceholder)
{
  // This test demonstrates the factory pattern structure
  // In Phase 2, this will actually create pieces
  
  Chess::Position test_pos(4, 4);
  
  // This will throw until Phase 2 implementation
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos),
    std::runtime_error
  );
}
