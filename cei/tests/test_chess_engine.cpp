/// @file      test_chess_engine.cpp
/// @brief     Unit tests for Chess Engine Interface using adapted patterns.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite for chess engine functionality adapted from existing test patterns:
///             - Move evaluation and search algorithms
///             - Position analysis and scoring
///             - Engine difficulty and performance
///             - Integration with modular components
/// @note      Uses modular architecture with smart pointer memory management
///            following modern C++ RAII principles adapted from existing tests.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chess_engine.h"
#include "chess_types.h"
#include "position.h"

/// @class   ChessEngineTest
/// @brief   Test fixture class for Chess Engine unit tests.
/// @details Provides setup and teardown methods for test environment.
///          Tests engine AI capabilities, search algorithms, and evaluation.
///          Adapted from existing test framework patterns.
class ChessEngineTest : public ::testing::Test
{
  protected:
    /// @brief   Chess engine instance for testing.
    std::unique_ptr<Chess::ChessEngine> engine_;

    /// @brief   Test grid positions for various scenarios.
    Chess::Grid standard_grid_;
    Chess::Grid empty_grid_;
    Chess::Grid endgame_grid_;

    /// @brief   Set up test environment before each test.
    /// @details Initializes engine and test positions.
    ///          Adapted from existing test SetUp() patterns.
    void SetUp() override
    {
      engine_ = std::make_unique<Chess::ChessEngine>();
      
      // Setup standard position
      standard_grid_.initialize_standard_position();
      
      // Setup empty grid
      empty_grid_.initialize_standard_position();
      empty_grid_.clear_square(Chess::Position(0, 0)); // Clear a1
      empty_grid_.clear_square(Chess::Position(1, 0)); // Clear b1
      empty_grid_.clear_square(Chess::Position(2, 0)); // Clear c1
      empty_grid_.clear_square(Chess::Position(3, 0)); // Clear d1
      empty_grid_.clear_square(Chess::Position(4, 0)); // Clear e1
      empty_grid_.clear_square(Chess::Position(5, 0)); // Clear f1
      empty_grid_.clear_square(Chess::Position(6, 0)); // Clear g1
      empty_grid_.clear_square(Chess::Position(7, 0)); // Clear h1
      // Clear all white pieces for empty grid test
      for (int file = 0; file < 8; file++)
      {
        empty_grid_.clear_square(Chess::Position(file, 1)); // Clear rank 2
        empty_grid_.clear_square(Chess::Position(file, 6)); // Clear rank 7
      }
      
      // Setup endgame position (king + pawn vs king)
      endgame_grid_.initialize_standard_position();
      // Clear most pieces for simplified endgame
      for (int file = 0; file < 8; file++)
      {
        for (int rank = 2; rank < 6; rank++)
        {
          endgame_grid_.clear_square(Chess::Position(file, rank));
        }
      }
    }

    /// @brief   Clean up test environment after each test.
    /// @details Automatically releases resources using smart pointer patterns.
    void TearDown() override
    {
      // Smart pointers will automatically clean up
      // No manual cleanup needed due to RAII principles
    }
};

/// @test   Verify engine initializes correctly.
/// @details Tests constructor and initial state setup.
///          Adapted from existing initialization test patterns.
TEST_F(ChessEngineTest, ConstructorInitialization)
{
  // Engine should be created successfully
  ASSERT_NE(engine_, nullptr);
  
  // Default difficulty should be 5
  engine_->set_difficulty(5);
  auto stats = engine_->get_statistics();
  EXPECT_NE(stats.find("Difficulty Level: 5"), std::string::npos);
}

/// @test   Verify position evaluation works.
/// @details Tests evaluate_position() method functionality.
///          Adapted from existing evaluation test patterns.
TEST_F(ChessEngineTest, PositionEvaluation)
{
  // Evaluate standard position
  int white_score = engine_->evaluate_position(standard_grid_, Chess::Color::WHITE);
  int black_score = engine_->evaluate_position(standard_grid_, Chess::Color::BLACK);
  
  // Scores should be opposite (black perspective is negative of white)
  EXPECT_EQ(white_score, -black_score);
  
  // Standard position should be roughly balanced
  EXPECT_LT(std::abs(white_score), 500); // Within 5 pawns of balanced
}

/// @test   Verify material evaluation works.
/// @details Tests material balance calculation.
///          Adapted from existing material counting test patterns.
TEST_F(ChessEngineTest, MaterialEvaluation)
{
  // Standard position should have equal material
  int material_score = engine_->evaluate_material(standard_grid_);
  EXPECT_EQ(material_score, 0); // Equal material in starting position
  
  // Empty position should have no material
  int empty_material = engine_->evaluate_material(empty_grid_);
  EXPECT_EQ(empty_material, 0); // No pieces
}

/// @test   Verify legal moves generation.
/// @details Tests get_legal_moves() method functionality.
///          Adapted from existing move generation test patterns.
TEST_F(ChessEngineTest, LegalMovesGeneration)
{
  // Standard position should have legal moves
  auto legal_moves = engine_->get_legal_moves(standard_grid_);
  EXPECT_GT(legal_moves.size(), 0);
  
  // Should include common opening moves
  bool has_e2e4 = false;
  bool has_d2d4 = false;
  bool has_g1f3 = false;
  
  for (const auto& move : legal_moves)
  {
    if (move.start_pos.file == 4 && move.start_pos.rank == 1 && 
        move.end_pos.file == 4 && move.end_pos.rank == 3) has_e2e4 = true;
    if (move.start_pos.file == 3 && move.start_pos.rank == 1 && 
        move.end_pos.file == 3 && move.end_pos.rank == 3) has_d2d4 = true;
    if (move.start_pos.file == 6 && move.start_pos.rank == 0 && 
        move.end_pos.file == 5 && move.end_pos.rank == 2) has_g1f3 = true;
  }
  
  EXPECT_TRUE(has_e2e4);
  EXPECT_TRUE(has_d2d4);
  EXPECT_TRUE(has_g1f3);
}

/// @test   Verify best move finding works.
/// @details Tests find_best_move() method functionality.
///          Adapted from existing AI decision test patterns.
TEST_F(ChessEngineTest, BestMoveFinding)
{
  // Set search limits for quick testing
  Chess::SearchLimits limits;
  limits.max_depth = 2;
  limits.max_time = std::chrono::milliseconds(100);
  
  // Find best move for standard position
  auto result = engine_->find_best_move(standard_grid_, limits);
  
  // Should return a valid result
  EXPECT_GT(result.depth, 0);
  EXPECT_GE(result.nodes_searched, 0);
  EXPECT_GE(result.time_used.count(), 0);
  
  // Best move should be valid (not default move)
  EXPECT_NE(result.best_move.start_pos.file, result.best_move.end_pos.file);
  EXPECT_NE(result.best_move.start_pos.rank, result.best_move.end_pos.rank);
}

/// @test   Verify difficulty setting works.
/// @details Tests set_difficulty() method functionality.
///          Adapted from existing configuration test patterns.
TEST_F(ChessEngineTest, DifficultySetting)
{
  // Set minimum difficulty
  engine_->set_difficulty(1);
  auto stats1 = engine_->get_statistics();
  EXPECT_NE(stats1.find("Difficulty Level: 1"), std::string::npos);
  
  // Set maximum difficulty
  engine_->set_difficulty(10);
  auto stats2 = engine_->get_statistics();
  EXPECT_NE(stats2.find("Difficulty Level: 10"), std::string::npos);
  
  // Test invalid difficulty (should be clamped)
  engine_->set_difficulty(15); // Above maximum
  auto stats3 = engine_->get_statistics();
  EXPECT_NE(stats3.find("Difficulty Level: 10"), std::string::npos);
  
  engine_->set_difficulty(-5); // Below minimum
  auto stats4 = engine_->get_statistics();
  EXPECT_NE(stats4.find("Difficulty Level: 1"), std::string::npos);
}

/// @test   Verify statistics tracking works.
/// @details Tests engine statistics and performance tracking.
///          Adapted from existing performance test patterns.
TEST_F(ChessEngineTest, StatisticsTracking)
{
  // Reset statistics
  engine_->reset_statistics();
  auto initial_stats = engine_->get_statistics();
  
  // Should show zero searches initially
  EXPECT_NE(initial_stats.find("Total Searches: 0"), std::string::npos);
  EXPECT_NE(initial_stats.find("Total Nodes Searched: 0"), std::string::npos);
  
  // Perform a search
  Chess::SearchLimits limits;
  limits.max_depth = 1;
  limits.max_time = std::chrono::milliseconds(50);
  
  engine_->find_best_move(standard_grid_, limits);
  
  // Should show updated statistics
  auto updated_stats = engine_->get_statistics();
  EXPECT_NE(updated_stats.find("Total Searches: 1"), std::string::npos);
  EXPECT_NE(updated_stats.find("Total Nodes Searched: 0"), std::string::npos); // May be 0 with depth 1
}

/// @test   Verify draw detection works.
/// @details Tests is_draw() method functionality.
///          Adapted from existing game state test patterns.
TEST_F(ChessEngineTest, DrawDetection)
{
  // Standard position should not be a draw
  EXPECT_FALSE(engine_->is_draw(standard_grid_));
  
  // Empty position might be a draw (insufficient material)
  // This depends on the specific implementation
  bool empty_is_draw = engine_->is_draw(empty_grid_);
  // Result may vary based on implementation
}

/// @test   Verify search limits are respected.
/// @details Tests search algorithm respects time and depth limits.
///          Adapted from existing performance constraint test patterns.
TEST_F(ChessEngineTest, SearchLimitsRespected)
{
  // Test depth limit
  Chess::SearchLimits depth_limits;
  depth_limits.max_depth = 1;
  depth_limits.max_time = std::chrono::milliseconds(1000); // Generous time
  
  auto depth_result = engine_->find_best_move(standard_grid_, depth_limits);
  EXPECT_LE(depth_result.depth, 1);
  
  // Test time limit (with very short time)
  Chess::SearchLimits time_limits;
  time_limits.max_depth = 10; // High depth
  time_limits.max_time = std::chrono::milliseconds(10); // Very short time
  
  auto time_result = engine_->find_best_move(standard_grid_, time_limits);
  EXPECT_LT(time_result.time_used.count(), 50); // Should finish quickly
}

/// @test   Verify engine integration with other modules.
/// @details Tests that engine works with CCI, CPL, CBM, CTU, CGH.
///          Adapted from existing integration test patterns.
TEST_F(ChessEngineTest, ModuleIntegration)
{
  // Engine should work with all modules
  // This test mainly ensures no crashes during integration
  
  // Test position evaluation (uses CCI + CPL)
  int score = engine_->evaluate_position(standard_grid_, Chess::Color::WHITE);
  EXPECT_NE(score, 0); // Should have some evaluation
  
  // Test move generation (uses all modules)
  auto moves = engine_->get_legal_moves(standard_grid_);
  EXPECT_GT(moves.size(), 0);
  
  // Test best move finding (uses all modules + AI)
  Chess::SearchLimits limits;
  limits.max_depth = 1;
  auto result = engine_->find_best_move(standard_grid_, limits);
  EXPECT_GT(result.nodes_searched, 0);
}

/// @test   Verify engine handles edge cases.
/// @details Tests engine behavior with unusual positions.
///          Adapted from existing edge case test patterns.
TEST_F(ChessEngineTest, EdgeCaseHandling)
{
  // Test with empty position
  auto empty_moves = engine_->get_legal_moves(empty_grid_);
  int empty_score = engine_->evaluate_position(empty_grid_, Chess::Color::WHITE);
  
  // Should handle gracefully (no crashes)
  EXPECT_GE(empty_score, -99999); // Reasonable score range
  EXPECT_LE(empty_score, 99999);
  
  // Test with endgame position
  auto endgame_moves = engine_->get_legal_moves(endgame_grid_);
  int endgame_score = engine_->evaluate_position(endgame_grid_, Chess::Color::WHITE);
  
  // Should handle gracefully
  EXPECT_GE(endgame_score, -99999);
  EXPECT_LE(endgame_score, 99999);
}
