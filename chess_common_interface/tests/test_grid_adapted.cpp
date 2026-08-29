/// @file      test_grid_adapted.cpp
/// @namespace Chess
/// @brief     Unit tests for CCI Grid structure.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite covering the Chess::Grid type:
///             - Standard-position initialization and piece count
///             - Position validity, equality, and board-coordinate helpers
///             - Turn switching, castling flags, and halfmove clock
///             - Piece-access, factory creation, apply_move_inplace, and undo_move
/// @note      Grid internals are heap-allocated via std::unique_ptr;
///            the fixture member is a value-type stack variable.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../include/grid.h"
#include "../include/position.h"
#include "../include/chess_types.h"

/// @class   GridTest
/// @brief   Test fixture class for Grid unit tests.
/// @details Provides a default-constructed Grid pre-initialized to the standard
///          opening position. Auxiliary PositionList and ColorList vectors are
///          reset to empty before every test case.
class GridTest : public ::testing::Test
{
  protected:
    /// @brief   Grid instance for testing.
    /// @details Uses standard grid structure with CCI namespace.
    Chess::Grid test_grid;

    /// @brief   Available moves vector for test results.
    /// @details Populated by a piece's available_moves() call during testing.
    Chess::PositionList moves = {};

    /// @brief   Expected moves vector for test validation.
    /// @details Contains the positions we expect pieces to be able to move to.
    Chess::PositionList expected_moves = {};

    /// @brief   Other pieces positions on the board for move validation.
    /// @details Represents positions of all pieces other than the one under test.
    Chess::PositionList other_pieces = {};

    /// @brief   Colors corresponding to other_pieces for capture validation.
    /// @details Parallel to other_pieces; used to determine which pieces can be captured.
    Chess::ColorList other_colors = {};

    /// @brief   Set up test environment before each test.
    /// @details Calls initialize_standard_position() on test_grid and resets
    ///          all auxiliary vectors to empty.
    void SetUp() override
    {
      // Initialize grid to standard position
      test_grid.initialize_standard_position();
      
      moves = {};
      expected_moves = {};
      other_pieces = {};
      other_colors = {};
    }

    /// @brief   Clean up test environment after each test.
    /// @details Grid and vector members are value types on the fixture; they
    ///          are destroyed automatically when the fixture goes out of scope.
    void TearDown() override
    {
    }
};

/// @test   Verify grid initialization creates correct piece count.
/// @details Confirms get_all_pieces() returns 32 after initialize_standard_position()
///          and that current_turn is WHITE.
TEST_F(GridTest, StandardPositionInitialization)
{
  // Test that standard position has correct number of pieces
  auto pieces = test_grid.get_all_pieces();
  
  // Standard chess starting position has 32 pieces
  EXPECT_EQ(pieces.size(), 32);
  
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
/// @details Checks that in-bounds positions (a1, h8, e4) report is_valid() == true
///          and out-of-bounds positions (-1,0), (8,8), (100,100) report false.
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
/// @details Checks == returns true for identical coordinates and != returns
///          true for positions that differ in rank or file.
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
/// @details Confirms that get_piece_at() and is_occupied() return null/false
///          for squares that are empty in the starting position.
TEST_F(GridTest, PieceAccessPatterns)
{
  // Test getting pieces at various positions
  Chess::Position pos_e4(4, 3);
  Chess::Position pos_empty(4, 4); // Center of empty board in Phase 1
  
  // These will be fully functional in Phase 2
  auto piece_e4 = test_grid.get_piece_at(pos_e4);
  auto piece_empty = test_grid.get_piece_at(pos_empty);
  
  // e4 is empty in the starting position.
  EXPECT_EQ(piece_e4, nullptr);
  EXPECT_EQ(piece_empty, nullptr);
  
  // Test grid occupation status
  EXPECT_FALSE(test_grid.is_occupied(pos_e4));
  EXPECT_FALSE(test_grid.is_occupied(pos_empty));
}

/// @test   Verify smart pointer ownership patterns work correctly.
/// @details Confirms the starting position contains 32 pieces and that
///          clearing the returned vector reduces it to zero without leaking.
TEST_F(GridTest, SmartPointerPatterns)
{
  // get_all_pieces() returns a vector of raw pointers into the grid's owned store.
  auto pieces = test_grid.get_all_pieces();
  
  // Starting position should contain all 32 pieces.
  EXPECT_EQ(pieces.size(), 32);
  
  // Clearing the returned copy does not affect the grid.
  pieces.clear();
  EXPECT_EQ(pieces.size(), 0);
}

/// @test   Verify grid turn management works correctly.
/// @details Confirms current_turn starts as WHITE and alternates correctly
///          across two switch_turn() calls.
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
/// @details Confirms all four castling-rights flags are true, halfmove_clock
///          is 0, and fullmove_number is 1 after initialize_standard_position().
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
/// @details Creates a PieceProperties value directly, sets each field, and
///          confirms all accessors reflect the assigned values.
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

/// @test   Verify factory creates a fully populated piece object.
/// @details Calls create_piece() for a white pawn at e5 and checks the
///          returned unique_ptr is non-null with correct type, color, and position.
TEST_F(GridTest, FactoryCreatesCorrectPiece)
{
  // This test demonstrates the factory pattern structure
  
  Chess::Position test_pos(4, 4);
  
  auto piece = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos);
  ASSERT_NE(piece, nullptr);
  EXPECT_EQ(piece->get_type(), Chess::PieceType::PAWN);
  EXPECT_EQ(piece->get_color(), Chess::Color::WHITE);
  EXPECT_EQ(piece->get_position(), test_pos);
}

TEST_F(GridTest, ApplyMoveInplaceResetsHalfmoveClockOnCapture)
{
  // Create a simple capture position: white pawn e4 captures black pawn d5.
  test_grid = Chess::Grid{};
  test_grid.flags.halfmove_clock = 17;
  test_grid.current_turn = Chess::Color::WHITE;

  test_grid.set_piece(
      Chess::Position(4, 3),
      Chess::PieceProperties{Chess::PieceType::PAWN, Chess::Color::WHITE, Chess::Position(4, 3), true, false});
  test_grid.set_piece(
      Chess::Position(3, 4),
      Chess::PieceProperties{Chess::PieceType::PAWN, Chess::Color::BLACK, Chess::Position(3, 4), true, false});

  const Chess::Move move(Chess::Position(4, 3), Chess::Position(3, 4));
  test_grid.apply_move_inplace(move);

  EXPECT_EQ(test_grid.flags.halfmove_clock, 0);
}

TEST_F(GridTest, ApplyUndoRestoresEnPassantVulnerabilityState)
{
  // White pawn on e4 is currently vulnerable; black pawn double-step should replace vulnerability,
  // and undo should restore original vulnerability map.
  test_grid = Chess::Grid{};
  test_grid.current_turn = Chess::Color::BLACK;

  test_grid.set_piece(
      Chess::Position(4, 3),
      Chess::PieceProperties{Chess::PieceType::PAWN, Chess::Color::WHITE, Chess::Position(4, 3), true, true});
  test_grid.set_piece(
      Chess::Position(6, 6),
      Chess::PieceProperties{Chess::PieceType::PAWN, Chess::Color::BLACK, Chess::Position(6, 6), false, false});

  const Chess::Move move(Chess::Position(6, 6), Chess::Position(6, 4));
  const Chess::UndoRecord undo = test_grid.apply_move_inplace(move);

  auto white_after_apply = test_grid.get_piece(Chess::Position(4, 3));
  auto black_after_apply = test_grid.get_piece(Chess::Position(6, 4));
  ASSERT_TRUE(white_after_apply.has_value());
  ASSERT_TRUE(black_after_apply.has_value());
  EXPECT_FALSE(white_after_apply->en_passant_vulnerable);
  EXPECT_TRUE(black_after_apply->en_passant_vulnerable);

  test_grid.undo_move(move, undo);

  auto white_after_undo = test_grid.get_piece(Chess::Position(4, 3));
  auto black_after_undo = test_grid.get_piece(Chess::Position(6, 6));
  ASSERT_TRUE(white_after_undo.has_value());
  ASSERT_TRUE(black_after_undo.has_value());
  EXPECT_TRUE(white_after_undo->en_passant_vulnerable);
  EXPECT_FALSE(black_after_undo->en_passant_vulnerable);
}
