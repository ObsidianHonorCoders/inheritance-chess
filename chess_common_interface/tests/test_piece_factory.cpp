/// @file      test_piece_factory.cpp
/// @namespace Chess
/// @brief     Unit tests for CCI Piece Factory.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite covering the create_piece() factory function:
///             - Successful construction of all six piece types
///             - Correct type, color, and position reported by the returned object
///             - Heap ownership via std::unique_ptr returned from the factory
///             - Corner, center, and edge position handling
///             - Both WHITE and BLACK color variants

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../include/grid.h"
#include "../include/position.h"
#include "../include/chess_types.h"

/// @class   PieceFactoryTest
/// @brief   Test fixture class for Piece Factory unit tests.
/// @details Pre-initializes a set of named Position values covering corners
///          (a1, h8), starting squares (e2, e7), minor-piece squares (b1, c1,
///          d1, e1), and the board center (e5).
class PieceFactoryTest : public ::testing::Test
{
  protected:
    /// @brief Test positions for piece creation.
    Chess::Position test_pos_a1;
    Chess::Position test_pos_b1;
    Chess::Position test_pos_c1;
    Chess::Position test_pos_d1;
    Chess::Position test_pos_e1;
    Chess::Position test_pos_e2;
    Chess::Position test_pos_e7;
    Chess::Position test_pos_h8;
    Chess::Position test_pos_center;
    /// @brief   Set up test environment before each test.
    /// @details Initializes the named test positions.
    void SetUp() override
    {
      // Named test positions.
      test_pos_a1 = Chess::Position(0, 0);   // a1
      test_pos_b1 = Chess::Position(1, 0);   // b1
      test_pos_c1 = Chess::Position(2, 0);   // c1
      test_pos_d1 = Chess::Position(3, 0);   // d1
      test_pos_e1 = Chess::Position(4, 0);   // e1
      test_pos_e2 = Chess::Position(4, 1);   // e2
      test_pos_e7 = Chess::Position(4, 6);   // e7
      test_pos_h8 = Chess::Position(7, 7);   // h8
      test_pos_center = Chess::Position(4, 4); // e5
    }

    /// @brief   Clean up test environment after each test.
    /// @details Position members are trivial value types; no explicit cleanup needed.
    void TearDown() override
    {
    }
};

/// @test   Verify factory creates all six piece types.
/// @details Calls create_piece() for PAWN, KNIGHT, BISHOP, ROOK, QUEEN, and KING
///          and asserts each returns a non-null unique_ptr.
TEST_F(PieceFactoryTest, FactoryCreatesAllPieceTypes)
{
  auto pawn   = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_e2);
  auto knight = Chess::create_piece(Chess::PieceType::KNIGHT, Chess::Color::WHITE, test_pos_b1);
  auto bishop = Chess::create_piece(Chess::PieceType::BISHOP, Chess::Color::WHITE, test_pos_c1);
  auto rook   = Chess::create_piece(Chess::PieceType::ROOK, Chess::Color::WHITE, test_pos_a1);
  auto queen  = Chess::create_piece(Chess::PieceType::QUEEN, Chess::Color::WHITE, test_pos_d1);
  auto king   = Chess::create_piece(Chess::PieceType::KING, Chess::Color::WHITE, test_pos_e1);

  ASSERT_NE(pawn, nullptr);
  ASSERT_NE(knight, nullptr);
  ASSERT_NE(bishop, nullptr);
  ASSERT_NE(rook, nullptr);
  ASSERT_NE(queen, nullptr);
  ASSERT_NE(king, nullptr);
}

/// @test   Verify factory returns a fully populated piece object.
/// @details Creates white and black pawns via create_piece() and asserts
///          get_type(), get_color(), and get_position() match the supplied arguments.
TEST_F(PieceFactoryTest, FactoryPatternStructure)
{
  // Verify the factory returns a complete object with correct properties.
  
  auto white_pawn = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_e2);
  ASSERT_NE(white_pawn, nullptr);
  EXPECT_EQ(white_pawn->get_type(), Chess::PieceType::PAWN);
  EXPECT_EQ(white_pawn->get_color(), Chess::Color::WHITE);
  EXPECT_EQ(white_pawn->get_position(), test_pos_e2);

  auto black_pawn = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::BLACK, test_pos_e7);
  ASSERT_NE(black_pawn, nullptr);
  EXPECT_EQ(black_pawn->get_type(), Chess::PieceType::PAWN);
  EXPECT_EQ(black_pawn->get_color(), Chess::Color::BLACK);
  EXPECT_EQ(black_pawn->get_position(), test_pos_e7);
}

/// @test   Verify factory returns a std::unique_ptr with exclusive ownership.
/// @details Creates a pawn via create_piece() and confirms the returned
///          unique_ptr is non-null, demonstrating heap ownership by the caller.
TEST_F(PieceFactoryTest, SmartPointerOwnershipPatterns)
{
  // Factory returns a unique_ptr; confirm heap allocation succeeded.
  
  auto piece = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_e2);
  ASSERT_NE(piece, nullptr);
}

/// @test   Verify position handling in factory interface.
/// @details Calls create_piece() with corner (a1), far-corner (h8), and
///          center (e5) positions and checks get_position() matches each.
TEST_F(PieceFactoryTest, PositionHandlingInFactory)
{
  // Verify positions are stored exactly as supplied.
  
  auto a1 = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_a1);
  auto h8 = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_h8);
  auto e5 = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_center);

  ASSERT_NE(a1, nullptr);
  ASSERT_NE(h8, nullptr);
  ASSERT_NE(e5, nullptr);
  EXPECT_EQ(a1->get_position(), test_pos_a1);
  EXPECT_EQ(h8->get_position(), test_pos_h8);
  EXPECT_EQ(e5->get_position(), test_pos_center);
}

/// @test   Verify color handling in factory interface.
/// @details Creates white and black pawns via create_piece() and confirms
///          get_color() returns the expected color for each.
TEST_F(PieceFactoryTest, ColorHandlingInFactory)
{
  // Both color variants must be returned with the correct color attribute.
  
  auto white_piece = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_e2);
  auto black_piece = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::BLACK, test_pos_e7);
  ASSERT_NE(white_piece, nullptr);
  ASSERT_NE(black_piece, nullptr);
  EXPECT_EQ(white_piece->get_color(), Chess::Color::WHITE);
  EXPECT_EQ(black_piece->get_color(), Chess::Color::BLACK);
}
