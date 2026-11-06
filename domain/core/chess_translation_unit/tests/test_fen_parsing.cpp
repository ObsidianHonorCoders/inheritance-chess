/// @file      test_fen_parsing.cpp
/// @namespace Chess
/// @brief     Unit tests for FEN notation parsing.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for Forsyth-Edwards Notation parsing and generation.

#include <gtest/gtest.h>
#include "../include/translation_unit.h"

using namespace Chess;

class FENParsingTest : public ::testing::Test
{
  protected:
    ChessTranslationUnit translator;

    void SetUp() override {}
};

TEST_F(FENParsingTest, StandardStartingPosition)
{
  const std::string starting_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  const Grid grid = translator.fen_to_internal(starting_fen);

  const auto white_king = grid.get_piece(Position(4, 0));
  const auto black_king = grid.get_piece(Position(4, 7));
  ASSERT_TRUE(white_king.has_value());
  ASSERT_TRUE(black_king.has_value());
  EXPECT_EQ(white_king->type, PieceType::KING);
  EXPECT_EQ(black_king->type, PieceType::KING);
  EXPECT_EQ(white_king->color, Color::WHITE);
  EXPECT_EQ(black_king->color, Color::BLACK);

  EXPECT_EQ(grid.current_turn, Color::WHITE);
  EXPECT_TRUE(grid.flags.white_can_castle_kingside);
  EXPECT_TRUE(grid.flags.white_can_castle_queenside);
  EXPECT_TRUE(grid.flags.black_can_castle_kingside);
  EXPECT_TRUE(grid.flags.black_can_castle_queenside);
  EXPECT_EQ(grid.flags.halfmove_clock, 0);
  EXPECT_EQ(grid.flags.fullmove_number, 1);

  EXPECT_EQ(translator.internal_to_fen(grid), starting_fen);
}

TEST_F(FENParsingTest, AfterOneMove)
{
  const std::string one_move_fen = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1";
  const Grid grid = translator.fen_to_internal(one_move_fen);

  const auto white_pawn_e4 = grid.get_piece(Position(4, 3));
  ASSERT_TRUE(white_pawn_e4.has_value());
  EXPECT_EQ(white_pawn_e4->type, PieceType::PAWN);
  EXPECT_EQ(white_pawn_e4->color, Color::WHITE);

  EXPECT_FALSE(grid.get_piece(Position(4, 1)).has_value());
  EXPECT_EQ(grid.current_turn, Color::BLACK);
  EXPECT_EQ(grid.flags.halfmove_clock, 0);
  EXPECT_EQ(grid.flags.fullmove_number, 1);

  EXPECT_EQ(translator.internal_to_fen(grid), one_move_fen);
}

TEST_F(FENParsingTest, ComplexPosition)
{
  const std::string complex_fen = "r3k2r/pp1n1ppp/2p1bn2/q2p4/3P4/2N1PN2/PPQ2PPP/R3KB1R b KQkq d3 12 20";
  const Grid grid = translator.fen_to_internal(complex_fen);

  EXPECT_EQ(grid.current_turn, Color::BLACK);
  EXPECT_TRUE(grid.flags.white_can_castle_kingside);
  EXPECT_TRUE(grid.flags.white_can_castle_queenside);
  EXPECT_TRUE(grid.flags.black_can_castle_kingside);
  EXPECT_TRUE(grid.flags.black_can_castle_queenside);
  EXPECT_EQ(grid.flags.halfmove_clock, 12);
  EXPECT_EQ(grid.flags.fullmove_number, 20);

  const auto black_queen_a5 = grid.get_piece(Position(0, 4));
  ASSERT_TRUE(black_queen_a5.has_value());
  EXPECT_EQ(black_queen_a5->type, PieceType::QUEEN);
  EXPECT_EQ(black_queen_a5->color, Color::BLACK);

  const auto white_knight_c3 = grid.get_piece(Position(2, 2));
  ASSERT_TRUE(white_knight_c3.has_value());
  EXPECT_EQ(white_knight_c3->type, PieceType::KNIGHT);
  EXPECT_EQ(white_knight_c3->color, Color::WHITE);

  // En passant target is d3, so d4 white pawn is vulnerable.
  const auto white_pawn_d4 = grid.get_piece(Position(3, 3));
  ASSERT_TRUE(white_pawn_d4.has_value());
  EXPECT_EQ(white_pawn_d4->type, PieceType::PAWN);
  EXPECT_TRUE(white_pawn_d4->en_passant_vulnerable);

  EXPECT_EQ(translator.internal_to_fen(grid), complex_fen);
}
