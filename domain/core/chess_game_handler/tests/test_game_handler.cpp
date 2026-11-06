/// @file      test_game_handler.cpp
/// @namespace Chess
/// @brief     Unit tests for CGH Game Handler.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Test suite covering ChessGameHandler behavior:
///             - Game lifecycle: new game, FEN load, move execution
///             - Legal move discovery including castling, en passant, and promotions
///             - Game-state transitions and draw-rule detection
///             - Clock management with start/stop timer semantics
/// @note      The handler_ member is a value-type fixture; heap-allocated
///            internals are managed by the handler’s own smart pointers.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <algorithm>
#include <thread>

#include "../include/game_handler.h"
#include "../../chess_common_interface/include/chess_types.h"
#include "../../chess_common_interface/include/position.h"

/// @class   GameHandlerTest
/// @brief   Test fixture class for Game Handler unit tests.
/// @details Provides a default-constructed ChessGameHandler for each test.
///          Tests cover game orchestration, FEN I/O, algebraic move parsing,
///          timer control, and multi-module state transitions.
class GameHandlerTest : public ::testing::Test
{
  protected:
    /// @brief   Game handler instance for testing.
    Chess::ChessGameHandler handler_;

    /// @brief   Set up test environment before each test.
    /// @details The handler_ is value-initialized by its default constructor;
    ///          no explicit initialization is required.
    void SetUp() override
    {
      // Game handler is created automatically by constructor
    }

    /// @brief   Clean up test environment after each test.
    /// @details ChessGameHandler owns its internal resources via smart pointers;
    ///          they are released when handler_ goes out of scope.
    void TearDown() override
    {
    }
};

/// @test   Verify game handler initializes correctly.
/// @details Confirms ChessGameHandler() constructs without throwing.
TEST_F(GameHandlerTest, ConstructorInitialization)
{
  // Game handler should be created successfully
  // This test mainly ensures the constructor doesn't crash
  SUCCEED(); // Constructor test - if we reach here, construction worked
}

/// @test   Verify new game starts with standard position.
/// @details Confirms start_new_game() sets state to ONGOING, returns a
///          non-empty FEN string, and embeds the expected back-rank piece letters.
TEST_F(GameHandlerTest, StartNewGame)
{
  // Start a new game
  handler_.start_new_game();
  
  // Game should be in ongoing state
  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::ONGOING);
  
  // Should have valid FEN for standard position
  std::string fen = handler_.get_current_fen();
  EXPECT_FALSE(fen.empty());
  EXPECT_NE(fen.find("rnbqkbnr"), std::string::npos); // Should contain back rank
  EXPECT_NE(fen.find("RNBQKBNR"), std::string::npos); // Should contain front rank
}

/// @test   Verify FEN loading functionality.
/// @details Loads the standard opening FEN via load_game_from_fen(), confirms
///          the state is ONGOING, and checks that get_current_fen() echoes
///          the exact FEN string back.
TEST_F(GameHandlerTest, LoadFromFEN)
{
  // Load game from FEN
  std::string test_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  handler_.load_game_from_fen(test_fen);
  
  // Game should be in ongoing state
  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::ONGOING);
  
  // FEN should match loaded position
  EXPECT_EQ(handler_.get_current_fen(), test_fen);
}

/// @test   Verify algebraic move execution.
/// @details Confirms make_move("e2e4") returns true, leaves the game ONGOING,
///          and produces a FEN with the pawn already at e4.
TEST_F(GameHandlerTest, MakeAlgebraicMove)
{
  // Start new game
  handler_.start_new_game();
  
  // Make a valid opening move
  EXPECT_TRUE(handler_.make_move("e2e4"));
  
  // Game should still be ongoing
  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::ONGOING);
  
  // FEN should reflect the move (Black to move after White's e2e4)
  std::string fen = handler_.get_current_fen();
  EXPECT_NE(fen.find("4P3"), std::string::npos); // Should show pawn at e4 (rank 4: 4 empty, P, 3 empty)
}

/// @test   Verify invalid move handling.
/// @details Confirms make_move("e2e5") returns false (illegal pawn jump) and
///          leaves game state unchanged as ONGOING.
TEST_F(GameHandlerTest, InvalidMoveHandling)
{
  // Start new game
  handler_.start_new_game();
  
  // Try invalid move
  EXPECT_FALSE(handler_.make_move("e2e5")); // Invalid pawn move
  
  // Game state should be unchanged
  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::ONGOING);
}

/// @test   Verify legal moves generation.
/// @details Confirms get_legal_moves() returns a non-empty list from the
///          starting position and includes e2-e4, d2-d4, and Ng1-f3 by
///          board-coordinate lookup.
TEST_F(GameHandlerTest, GetLegalMoves)
{
  // Start new game
  handler_.start_new_game();
  
  // Get legal moves for white
  auto legal_moves = handler_.get_legal_moves();
  
  // Should have some legal moves from starting position
  EXPECT_GT(legal_moves.size(), 0);
  
  // Should include common opening moves
  bool has_e2e4 = false;
  bool has_d2d4 = false;
  bool has_g1f3 = false;
  
  for (const auto& move : legal_moves)
  {
    // Validate known opening moves by board coordinates.
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

/// @test   Verify game state transitions.
/// @details Confirms initial state is ONGOING, remains ONGOING after one
///          legal pawn move, and the state accessor reflects each transition.
TEST_F(GameHandlerTest, GameStateTransitions)
{
  // Start new game
  handler_.start_new_game();
  
  // Initial state should be ongoing
  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::ONGOING);
  
  // Make a move
  handler_.make_move("e2e4");
  
  // State should still be ongoing (no checkmate/stalemate yet)
  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::ONGOING);
}

/// @test   Verify time management functionality.
/// @details Confirms get_time_remaining() returns the default 10-minute
///          allocation (600 000 ms) for both WHITE and BLACK on a fresh game.
TEST_F(GameHandlerTest, TimeManagement)
{
  // Start new game
  handler_.start_new_game();
  
  // Should have default time for both players
  auto white_time = handler_.get_time_remaining(Chess::Color::WHITE);
  auto black_time = handler_.get_time_remaining(Chess::Color::BLACK);
  
  EXPECT_EQ(white_time, std::chrono::milliseconds(600000)); // 10 minutes
  EXPECT_EQ(black_time, std::chrono::milliseconds(600000)); // 10 minutes
}

TEST_F(GameHandlerTest, TimerStartStopUpdatesRemainingTime)
{
  handler_.start_new_game();

  const auto initial_white = handler_.get_time_remaining(Chess::Color::WHITE);
  handler_.start_timer(Chess::Color::WHITE);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  handler_.stop_timer(Chess::Color::WHITE);

  const auto after_white = handler_.get_time_remaining(Chess::Color::WHITE);
  EXPECT_LT(after_white, initial_white);

  const auto stable_white = handler_.get_time_remaining(Chess::Color::WHITE);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  const auto stable_white_after_wait = handler_.get_time_remaining(Chess::Color::WHITE);
  EXPECT_EQ(stable_white, stable_white_after_wait);
}

/// @test   Verify component integration.
/// @details Exercises make_move() (CBM + CPL), get_current_fen() (CTU), and
///          get_legal_moves() (CPL + CBM) on a fresh game to confirm all
///          module boundaries cooperate correctly.
TEST_F(GameHandlerTest, ComponentIntegration)
{
  handler_.start_new_game();

  // All module boundaries: move execution, FEN serialization, move generation.
  
  // Should be able to make moves (requires all modules)
  EXPECT_TRUE(handler_.make_move("e2e4"));
  
  // Should be able to get FEN (requires CTU)
  std::string fen = handler_.get_current_fen();
  EXPECT_FALSE(fen.empty());
  
  // Should be able to get legal moves (requires CPL + CBM)
  auto moves = handler_.get_legal_moves();
  EXPECT_GT(moves.size(), 0);
}

/// @test   Verify display functionality.
/// @details Confirms display_board() completes without throwing for a fresh game.
TEST_F(GameHandlerTest, DisplayBoard)
{
  // Start new game
  handler_.start_new_game();
  
  // Display should not crash (visual verification)
  EXPECT_NO_THROW(handler_.display_board());
  
  // This test mainly ensures the display method works without errors
  // Visual verification would be done manually
}

TEST_F(GameHandlerTest, LegalMovesAutoDiscoverCastlingFlags)
{
  handler_.load_game_from_fen("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1");

  const auto legal_moves = handler_.get_legal_moves();

  const auto has_kingside = std::find(
      legal_moves.begin(),
      legal_moves.end(),
      Chess::Move(Chess::Position(4, 0), Chess::Position(6, 0), Chess::SpecialFlags::CASTLE_KINGSIDE))
      != legal_moves.end();

  const auto has_queenside = std::find(
      legal_moves.begin(),
      legal_moves.end(),
      Chess::Move(Chess::Position(4, 0), Chess::Position(2, 0), Chess::SpecialFlags::CASTLE_QUEENSIDE))
      != legal_moves.end();

  EXPECT_TRUE(has_kingside);
  EXPECT_TRUE(has_queenside);
}

TEST_F(GameHandlerTest, LegalMovesAutoDiscoverEnPassantFlag)
{
  handler_.load_game_from_fen("4k3/8/8/3pP3/8/8/8/4K3 w - - 0 1");

  const auto legal_moves = handler_.get_legal_moves();

  const auto has_en_passant = std::find(
      legal_moves.begin(),
      legal_moves.end(),
      Chess::Move(Chess::Position(4, 4), Chess::Position(3, 5), Chess::SpecialFlags::EN_PASSANT))
      != legal_moves.end();

  EXPECT_TRUE(has_en_passant);
}

TEST_F(GameHandlerTest, LegalMovesAutoDiscoverPromotionVariants)
{
  handler_.load_game_from_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");

  const auto legal_moves = handler_.get_legal_moves();

  const Chess::Position start(0, 6);
  const Chess::Position end(0, 7);

  EXPECT_TRUE(std::find(legal_moves.begin(), legal_moves.end(), Chess::Move(start, end, Chess::SpecialFlags::PROMOTION, Chess::PieceType::QUEEN)) != legal_moves.end());
  EXPECT_TRUE(std::find(legal_moves.begin(), legal_moves.end(), Chess::Move(start, end, Chess::SpecialFlags::PROMOTION, Chess::PieceType::ROOK)) != legal_moves.end());
  EXPECT_TRUE(std::find(legal_moves.begin(), legal_moves.end(), Chess::Move(start, end, Chess::SpecialFlags::PROMOTION, Chess::PieceType::BISHOP)) != legal_moves.end());
  EXPECT_TRUE(std::find(legal_moves.begin(), legal_moves.end(), Chess::Move(start, end, Chess::SpecialFlags::PROMOTION, Chess::PieceType::KNIGHT)) != legal_moves.end());
}

TEST_F(GameHandlerTest, FiftyMoveRuleSetsSpecificDrawState)
{
  // Load position where halfmove clock is already at 100 and only a rook remains
  // (insufficient-material check does NOT fire for rook positions, so fifty-move
  // is the sole matching condition). CGH does not currently manage the halfmove
  // counter itself; is_draw tests the flag as-loaded after any subsequent move.
  handler_.load_game_from_fen("4k3/8/8/8/8/8/8/4KR2 w - - 100 1");

  EXPECT_TRUE(handler_.make_move("f1f2"));
  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::DRAW_FIFTY_MOVE_RULE);
}

TEST_F(GameHandlerTest, ThreefoldRepetitionSetsSpecificDrawState)
{
  handler_.start_new_game();

  EXPECT_TRUE(handler_.make_move("g1f3"));
  EXPECT_TRUE(handler_.make_move("g8f6"));
  EXPECT_TRUE(handler_.make_move("f3g1"));
  EXPECT_TRUE(handler_.make_move("f6g8"));

  EXPECT_TRUE(handler_.make_move("g1f3"));
  EXPECT_TRUE(handler_.make_move("g8f6"));
  EXPECT_TRUE(handler_.make_move("f3g1"));
  EXPECT_TRUE(handler_.make_move("f6g8"));

  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::DRAW_THREEFOLD_REPETITION);
}

TEST_F(GameHandlerTest, DrawOfferAcceptTransitionsToAgreement)
{
  handler_.start_new_game();

  EXPECT_TRUE(handler_.offer_draw(Chess::Color::WHITE));
  EXPECT_TRUE(handler_.is_draw_offered_by(Chess::Color::WHITE));
  EXPECT_FALSE(handler_.is_draw_offered_by(Chess::Color::BLACK));

  EXPECT_TRUE(handler_.accept_draw(Chess::Color::BLACK));
  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::DRAW_BY_AGREEMENT);
}

TEST_F(GameHandlerTest, AcceptDrawWithoutOfferFails)
{
  handler_.start_new_game();
  EXPECT_FALSE(handler_.accept_draw(Chess::Color::WHITE));
  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::ONGOING);
}

TEST_F(GameHandlerTest, ResignTransitionsState)
{
  handler_.start_new_game();
  EXPECT_TRUE(handler_.resign(Chess::Color::WHITE));
  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::WHITE_RESIGNED);
}

TEST_F(GameHandlerTest, TimeoutTransitionsState)
{
  handler_.start_new_game();
  EXPECT_TRUE(handler_.declare_timeout(Chess::Color::BLACK));
  EXPECT_EQ(handler_.get_game_state(), Chess::GameState::BLACK_TIMEOUT);
}

TEST_F(GameHandlerTest, MoveRejectedAfterTerminalTransition)
{
  handler_.start_new_game();
  ASSERT_TRUE(handler_.resign(Chess::Color::BLACK));
  EXPECT_FALSE(handler_.make_move("e2e4"));
}
