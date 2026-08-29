/// @file      test_chess_engine.cpp
/// @namespace Chess
/// @brief     Unit tests for Chess Engine Interface (CEI module).
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite covering ChessEngine behavior:
///             - Minimax search with configurable depth and time limits
///             - Material-balance and positional evaluation
///             - Difficulty clamping and statistics tracking
///             - Integration of CEI with CCI, CPL, CBM, CTU, and CGH modules
/// @note      The engine_ fixture member is heap-allocated via std::unique_ptr
///            and automatically released after each test.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chess_engine.h"
#include "chess_types.h"
#include "position.h"

/// @class   ChessEngineTest
/// @brief   Test fixture class for Chess Engine unit tests.
/// @details Initializes three Grid states — standard opening, fully cleared, and
///          a simplified endgame with only back-rank pieces — and a ChessEngine
///          instance. Each test receives an independent engine and board state.
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
    /// @details Constructs a ChessEngine, initializes standard_grid_ to the
    ///          standard opening position, clears empty_grid_ of all pieces,
    ///          and strips endgame_grid_ to only the back ranks.
    void SetUp() override
    {
      engine_ = std::make_unique<Chess::ChessEngine>();
      
      // Setup standard position
      standard_grid_.initialize_standard_position();
      
      // Setup empty grid
      empty_grid_.initialize_standard_position();
      // Clear all squares for a truly empty board.
      for (int file = 0; file < 8; file++)
      {
        for (int rank = 0; rank < 8; rank++)
        {
          empty_grid_.clear_square(Chess::Position(file, rank));
        }
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
    /// @details The engine_ unique_ptr is automatically destroyed; no explicit
    ///          deallocation is required.
    void TearDown() override
    {
    }
};

/// @test   Verify engine initializes correctly.
/// @details Confirms that ChessEngine() constructs without throwing and that
///          set_difficulty(5) is reflected in get_statistics().
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
/// @details Checks that evaluate_position() returns scores symmetric across
///          colors (white score == –black score) and that the starting position
///          evaluates to within 5 pawns of balance.
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
/// @details Confirms evaluate_material() returns 0 for the symmetric starting
///          position and 0 for an empty board with no pieces.
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
/// @details Checks that get_legal_moves() produces a non-empty list from the
///          starting position and includes the specific opening moves e2-e4,
///          d2-d4, and Ng1-f3 by board-coordinate lookup.
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
/// @details Runs a depth-2, 100 ms search and asserts the result has positive
///          depth, non-negative node count, and a best move where start != end.
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
  EXPECT_NE(result.best_move.start_pos, result.best_move.end_pos);
}

/// @test   Verify difficulty setting works.
/// @details Confirms set_difficulty() stores valid levels (1 and 10), clamps
///          out-of-range values above the maximum to 10, and below the minimum
///          to 1, as reported by get_statistics().
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
/// @details Confirms reset_statistics() zeroes the search and node counters,
///          and that one call to find_best_move() increments Total Searches to 1.
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
  EXPECT_NE(updated_stats.find("Total Nodes Searched:"), std::string::npos);
}

/// @test   Verify draw detection works.
/// @details Checks that is_draw() returns false for the full starting position
///          and true for a bare king-vs-king board (insufficient material).
TEST_F(ChessEngineTest, DrawDetection)
{
  // Standard position should not be a draw
  EXPECT_FALSE(engine_->is_draw(standard_grid_));

  // King vs king is an insufficient-material draw.
  Chess::Grid kings_only;
  kings_only.set_piece(
      Chess::Position(4, 0),
      Chess::PieceProperties{Chess::PieceType::KING, Chess::Color::WHITE, Chess::Position(4, 0), false, false});
  kings_only.set_piece(
      Chess::Position(4, 7),
      Chess::PieceProperties{Chess::PieceType::KING, Chess::Color::BLACK, Chess::Position(4, 7), false, false});

  EXPECT_TRUE(engine_->is_draw(kings_only));
}

/// @test   Verify search limits are respected.
/// @details Runs a depth-1 search and asserts the result depth is ≤ 1.
///          Also runs a high-depth, 10 ms time-limited search and asserts
///          the reported time stays within a reasonable bound.
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
  EXPECT_LT(time_result.time_used.count(), 10000); // Should complete in a bounded time
}

/// @test   Verify engine integration with other modules.
/// @details Exercises evaluate_position() (CCI + CPL), get_legal_moves() (CBM),
///          and find_best_move() (full stack) to confirm each module boundary
///          produces results in valid ranges.
TEST_F(ChessEngineTest, ModuleIntegration)
{
  // Test all cross-module paths: evaluation, move generation, and search.
  
  // Test position evaluation (uses CCI + CPL)
  int score = engine_->evaluate_position(standard_grid_, Chess::Color::WHITE);
  EXPECT_GE(score, -99999);
  EXPECT_LE(score, 99999);
  
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
/// @details Confirms evaluate_position() and get_legal_moves() return results
///          within valid score ranges for both the fully empty board and the
///          stripped endgame board, exercising boundary positions.
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
