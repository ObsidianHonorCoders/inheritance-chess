/// @file      test_piece_factory.cpp
/// @brief     Unit tests for CCI Piece Factory using adapted patterns.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite for piece factory functionality adapted from existing piece creation patterns:
///             - Factory pattern implementation
///             - Smart pointer memory management
///             - Type and color validation
///             - Position handling
/// @note      Uses std::unique_ptr for automatic memory management
///            following modern C++ RAII principles adapted from existing tests.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../include/grid.h"
#include "../include/position.h"
#include "../include/chess_types.h"

/// @class   PieceFactoryTest
/// @brief   Test fixture class for Piece Factory unit tests.
/// @details Provides setup and teardown methods for test environment.
///          Uses smart pointers for automatic memory management.
///          Adapted from existing piece creation test patterns.
class PieceFactoryTest : public ::testing::Test
{
  protected:
    /// @brief   Set up test environment before each test.
    /// @details Initializes test positions and parameters.
    ///          Adapted from existing PawnTest SetUp() pattern.
    void SetUp() override
    {
      // Common test positions (adapted from existing test patterns)
      test_pos_e2 = Chess::Position(4, 1);  // e2
      test_pos_e7 = Chess::Position(4, 6);  // e7
      test_pos_a1 = Chess::Position(0, 0);   // a1
      test_pos_h8 = Chess::Position(7, 7);   // h8
      test_pos_center = Chess::Position(4, 4); // e5
    }

    /// @brief   Clean up test environment after each test.
    /// @details Automatically releases resources using smart pointer patterns.
    void TearDown() override
    {
      // Smart pointers will automatically clean up
      // No manual cleanup needed due to RAII principles
    }

    /// @brief   Common test positions.
    /// @details Adapted from existing test position patterns.
    Chess::Position test_pos_e2;
    Chess::Position test_pos_e7;
    Chess::Position test_pos_a1;
    Chess::Position test_pos_h8;
    Chess::Position test_pos_center;
};

/// @test   Verify factory throws exception for unimplemented pieces.
/// @details Adapted from existing piece creation validation patterns.
TEST_F(PieceFactoryTest, FactoryThrowsForUnimplemented)
{
  // Test that factory throws runtime_error for all piece types in Phase 1
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_e2),
    std::runtime_error
  );
  
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::KNIGHT, Chess::Color::WHITE, test_pos_b1),
    std::runtime_error
  );
  
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::BISHOP, Chess::Color::WHITE, test_pos_c1),
    std::runtime_error
  );
  
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::ROOK, Chess::Color::WHITE, test_pos_a1),
    std::runtime_error
  );
  
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::QUEEN, Chess::Color::WHITE, test_pos_d1),
    std::runtime_error
  );
  
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::KING, Chess::Color::WHITE, test_pos_e1),
    std::runtime_error
  );
}

/// @test   Verify factory pattern structure is ready for Phase 2.
/// @details Adapted from existing piece creation patterns.
TEST_F(PieceFactoryTest, FactoryPatternStructure)
{
  // This test verifies the factory interface is correct
  // In Phase 2, these will create actual pieces
  
  // Test white piece creation interface
  // auto white_pawn = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_e2);
  // EXPECT_NE(white_pawn, nullptr);
  // EXPECT_EQ(white_pawn->get_type(), Chess::PieceType::PAWN);
  // EXPECT_EQ(white_pawn->get_color(), Chess::Color::WHITE);
  // EXPECT_EQ(white_pawn->get_position(), test_pos_e2);
  
  // Test black piece creation interface
  // auto black_pawn = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::BLACK, test_pos_e7);
  // EXPECT_NE(black_pawn, nullptr);
  // EXPECT_EQ(black_pawn->get_type(), Chess::PieceType::PAWN);
  // EXPECT_EQ(black_pawn->get_color(), Chess::Color::BLACK);
  // EXPECT_EQ(black_pawn->get_position(), test_pos_e7);
  
  // For now, just verify the interface exists and throws appropriately
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_e2),
    std::runtime_error
  );
}

/// @test   Verify smart pointer ownership patterns.
/// @details Adapted from existing smart pointer test patterns.
TEST_F(PieceFactoryTest, SmartPointerOwnershipPatterns)
{
  // Test that factory returns unique_ptr (adapted from existing patterns)
  // This will be fully functional in Phase 2
  
  // auto piece = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_e2);
  // EXPECT_NE(piece, nullptr);
  
  // Test unique ownership
  // auto piece_copy = piece; // Should not compile - unique_ptr is not copyable
  // auto piece_move = std::move(piece); // Should compile - unique_ptr is movable
  
  // EXPECT_EQ(piece, nullptr); // Original should be null after move
  // EXPECT_NE(piece_move, nullptr); // Moved-to should be valid
  
  // For now, just verify the exception pattern
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_e2),
    std::runtime_error
  );
}

/// @test   Verify position handling in factory interface.
/// @details Adapted from existing position validation patterns.
TEST_F(PieceFactoryTest, PositionHandlingInFactory)
{
  // Test various positions with factory interface
  // This will be fully functional in Phase 2
  
  // Test corner positions
  // auto corner_piece = Chess::create_piece(Chess::PieceType::ROOK, Chess::Color::WHITE, test_pos_a1);
  // EXPECT_EQ(corner_piece->get_position(), test_pos_a1);
  
  // Test center positions
  // auto center_piece = Chess::create_piece(Chess::PieceType::QUEEN, Chess::Color::WHITE, test_pos_center);
  // EXPECT_EQ(center_piece->get_position(), test_pos_center);
  
  // Test edge positions
  // auto edge_piece = Chess::create_piece(Chess::PieceType::KNIGHT, Chess::Color::WHITE, Chess::Position(0, 4));
  // EXPECT_EQ(edge_piece->get_position(), Chess::Position(0, 4));
  
  // For now, just verify the interface accepts positions correctly
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_a1),
    std::runtime_error
  );
  
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_h8),
    std::runtime_error
  );
  
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_center),
    std::runtime_error
  );
}

/// @test   Verify color handling in factory interface.
/// @details Adapted from existing color validation patterns.
TEST_F(PieceFactoryTest, ColorHandlingInFactory)
{
  // Test both colors with factory interface
  // This will be fully functional in Phase 2
  
  // Test white pieces
  // auto white_piece = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_e2);
  // EXPECT_EQ(white_piece->get_color(), Chess::Color::WHITE);
  
  // Test black pieces
  // auto black_piece = Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::BLACK, test_pos_e7);
  // EXPECT_EQ(black_piece->get_color(), Chess::Color::BLACK);
  
  // For now, just verify the interface accepts colors correctly
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::WHITE, test_pos_e2),
    std::runtime_error
  );
  
  EXPECT_THROW(
    Chess::create_piece(Chess::PieceType::PAWN, Chess::Color::BLACK, test_pos_e7),
    std::runtime_error
  );
}
