/// @file      test_translation_unit.cpp
/// @namespace Chess
/// @brief     Unit tests for Chess Translation Unit.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Tests for notation conversion and parsing.

#include <gtest/gtest.h>
#include "../../chess_engine_interface/include/chess_engine.h"
#include "../../chess_game_handler/include/game_handler.h"
#include "../include/translation_unit.h"

using namespace Chess;

class TranslationUnitTest : public ::testing::Test
{
  protected:
    ChessTranslationUnit translator;

    void SetUp() override {}
};

TEST_F(TranslationUnitTest, FENToInternal)
{
  const std::string fen = "r3k2r/8/8/3pP3/8/8/8/R3K2R w Kq d6 0 1";
  Grid grid = translator.fen_to_internal(fen);

  const auto black_pawn = grid.get_piece(Position(3, 4));
  ASSERT_TRUE(black_pawn.has_value());
  EXPECT_EQ(black_pawn->type, PieceType::PAWN);
  EXPECT_TRUE(black_pawn->has_moved);
  EXPECT_TRUE(black_pawn->en_passant_vulnerable);

  const auto white_king = grid.get_piece(Position(4, 0));
  const auto white_rook_a = grid.get_piece(Position(0, 0));
  const auto white_rook_h = grid.get_piece(Position(7, 0));
  const auto black_king = grid.get_piece(Position(4, 7));
  const auto black_rook_a = grid.get_piece(Position(0, 7));
  const auto black_rook_h = grid.get_piece(Position(7, 7));

  ASSERT_TRUE(white_king.has_value());
  ASSERT_TRUE(white_rook_a.has_value());
  ASSERT_TRUE(white_rook_h.has_value());
  ASSERT_TRUE(black_king.has_value());
  ASSERT_TRUE(black_rook_a.has_value());
  ASSERT_TRUE(black_rook_h.has_value());

  EXPECT_FALSE(white_king->has_moved);
  EXPECT_TRUE(white_rook_a->has_moved);
  EXPECT_FALSE(white_rook_h->has_moved);
  EXPECT_FALSE(black_king->has_moved);
  EXPECT_FALSE(black_rook_a->has_moved);
  EXPECT_TRUE(black_rook_h->has_moved);
}

TEST_F(TranslationUnitTest, InternalToFEN)
{
  const std::string fen = "r3k2r/8/8/3pP3/8/8/8/R3K2R w Kq d6 0 1";
  Grid grid = translator.fen_to_internal(fen);

  EXPECT_EQ(translator.internal_to_fen(grid), fen);
}

TEST_F(TranslationUnitTest, MoveToSANAndBack)
{
  Grid grid;
  grid.initialize_standard_position();

  const Move e2e4 = translator.algebraic_to_move("e2e4", grid);
  const std::string san_e4 = translator.move_to_san(e2e4, grid);
  EXPECT_EQ(san_e4, "e4");

  const Move parsed_e4 = translator.san_to_move("e4", grid);
  EXPECT_EQ(parsed_e4, e2e4);

  grid.apply_move_inplace(e2e4);
  const Move g8f6 = translator.algebraic_to_move("g8f6", grid);
  const std::string san_nf6 = translator.move_to_san(g8f6, grid);
  EXPECT_EQ(san_nf6, "Nf6");

  const Move parsed_nf6 = translator.san_to_move("Nf6", grid);
  EXPECT_EQ(parsed_nf6, g8f6);
}

TEST_F(TranslationUnitTest, SANCastling)
{
  Grid grid = translator.fen_to_internal("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

  const Move kingside_castle = translator.san_to_move("O-O", grid);
  EXPECT_TRUE(kingside_castle.is_castling());
  EXPECT_EQ(kingside_castle.flags, SpecialFlags::CASTLE_KINGSIDE);
  EXPECT_EQ(translator.move_to_san(kingside_castle, grid), "O-O");
}

TEST_F(TranslationUnitTest, SANPromotion)
{
  Grid grid = translator.fen_to_internal("k7/4P3/8/8/8/8/8/7K w - - 0 1");

  const Move promotion_move = translator.san_to_move("e8=Q", grid);
  EXPECT_TRUE(promotion_move.is_promotion());
  EXPECT_EQ(promotion_move.start_pos, Position(4, 6));
  EXPECT_EQ(promotion_move.end_pos, Position(4, 7));
  EXPECT_EQ(promotion_move.promotion_piece, PieceType::QUEEN);
  EXPECT_EQ(translator.move_to_san(promotion_move, grid), "e8=Q+");
}

TEST_F(TranslationUnitTest, SANPawnCapture)
{
  Grid grid = translator.fen_to_internal("8/8/8/3p4/4P3/8/8/4K2k w - - 0 1");

  const Move capture = translator.san_to_move("exd5", grid);
  EXPECT_EQ(capture.start_pos, Position(4, 3));
  EXPECT_EQ(capture.end_pos, Position(3, 4));
  EXPECT_EQ(translator.move_to_san(capture, grid), "exd5");
}

TEST_F(TranslationUnitTest, PGNExportImportRoundTrip)
{
  Grid grid;
  grid.initialize_standard_position();

  std::vector<Move> moves;
  moves.push_back(translator.algebraic_to_move("e2e4", grid));
  grid.apply_move_inplace(moves.back());
  moves.push_back(translator.algebraic_to_move("e7e5", grid));
  grid.apply_move_inplace(moves.back());
  moves.push_back(translator.algebraic_to_move("g1f3", grid));
  grid.apply_move_inplace(moves.back());
  moves.push_back(translator.algebraic_to_move("b8c6", grid));

  Grid initial;
  initial.initialize_standard_position();

  const std::string pgn = translator.moves_to_pgn(moves, initial, "*");
  EXPECT_EQ(pgn, "1. e4 e5 2. Nf3 Nc6 *");

  const std::vector<Move> parsed = translator.pgn_to_moves(pgn, initial);
  ASSERT_EQ(parsed.size(), moves.size());
  for (std::size_t i = 0; i < moves.size(); ++i)
  {
    EXPECT_EQ(parsed[i], moves[i]);
  }
}

TEST_F(TranslationUnitTest, RichPGNParseAndSerializePreservesTagsCommentsAndVariations)
{
  Grid initial;
  initial.initialize_standard_position();

  const std::string pgn =
      "[Event \"Test Game\"]\n"
      "[Site \"Local\"]\n"
      "[Result \"1-0\"]\n\n"
      "1. e4 {King's pawn opening} (1. d4 d5) e5 2. Nf3 Nc6 1-0";

  const PGNGame game = translator.parse_pgn_game(pgn, initial);
  ASSERT_EQ(game.tags.size(), 3u);
  EXPECT_EQ(game.tags[0].first, "Event");
  EXPECT_EQ(game.tags[0].second, "Test Game");
  ASSERT_EQ(game.moves.size(), 4u);
  ASSERT_EQ(game.moves[0].comments.size(), 1u);
  EXPECT_EQ(game.moves[0].comments[0], "King's pawn opening");
  ASSERT_EQ(game.moves[0].variations.size(), 1u);
  ASSERT_EQ(game.moves[0].variations[0].moves.size(), 2u);
  EXPECT_EQ(game.moves[0].variations[0].moves[0].san, "d4");
  EXPECT_EQ(game.result, "1-0");

  const std::string serialized = translator.pgn_game_to_string(game, initial);
  EXPECT_NE(serialized.find("[Event \"Test Game\"]"), std::string::npos);
  EXPECT_NE(serialized.find("{King's pawn opening}"), std::string::npos);
  EXPECT_NE(serialized.find("(1. d4 d5)"), std::string::npos);
  EXPECT_NE(serialized.find("1-0"), std::string::npos);
}

TEST_F(TranslationUnitTest, PGNParsesExtendedAnnotationSymbols)
{
  Grid initial;
  initial.initialize_standard_position();

  const std::string pgn = "1. e4 !? e5 $2 2. Nf3!! Nc6?! *";
  const PGNGame game = translator.parse_pgn_game(pgn, initial);

  ASSERT_EQ(game.moves.size(), 4u);
  ASSERT_EQ(game.moves[0].annotations.size(), 1u);
  EXPECT_EQ(game.moves[0].annotations[0], "!?");
  ASSERT_EQ(game.moves[1].annotations.size(), 1u);
  EXPECT_EQ(game.moves[1].annotations[0], "$2");
  ASSERT_EQ(game.moves[2].annotations.size(), 1u);
  EXPECT_EQ(game.moves[2].annotations[0], "!!");
  ASSERT_EQ(game.moves[3].annotations.size(), 1u);
  EXPECT_EQ(game.moves[3].annotations[0], "?!");
}

TEST_F(TranslationUnitTest, PGNRecoversMalformedTagLinesWhenEnabled)
{
  Grid initial;
  initial.initialize_standard_position();

  const std::string pgn =
      "[Event \"Broken Tag]\n"
      "[Site Local]\n"
      "[Result \"1-0\"]\n\n"
      "1. e4 e5 1-0";

  PGNParseOptions recover_options;
  recover_options.recover_malformed_tags = true;
  recover_options.strict_tag_parsing = false;
  const PGNGame recovered = translator.parse_pgn_game(pgn, initial, recover_options);
  ASSERT_EQ(recovered.tags.size(), 3u);
  EXPECT_EQ(recovered.tags[0].first, "Event");
  EXPECT_EQ(recovered.tags[1].first, "Site");
  EXPECT_EQ(recovered.tags[2].first, "Result");
  EXPECT_EQ(recovered.malformed_tag_lines.size(), 2u);

  PGNParseOptions strict_options;
  strict_options.recover_malformed_tags = false;
  strict_options.strict_tag_parsing = true;
  const PGNGame strict = translator.parse_pgn_game(pgn, initial, strict_options);
  EXPECT_TRUE(strict.tags.empty());
  EXPECT_TRUE(strict.moves.empty());
}

TEST_F(TranslationUnitTest, PGNStrictNormalizationSortsTagsAndCanonicalizesAnnotations)
{
  Grid initial;
  initial.initialize_standard_position();

  const std::string pgn =
      "[Black \"B\"]\n"
      "[Event \"E\"]\n"
      "[White \"W\"]\n\n"
      "1. e4 ! {  king   side   } e5 ? *";

  const PGNGame game = translator.parse_pgn_game(pgn, initial);

  PGNSerializeOptions options;
  options.strict_normalization = true;
  const std::string normalized = translator.pgn_game_to_string(game, initial, options);

  const auto event_pos = normalized.find("[Event \"E\"]");
  const auto white_pos = normalized.find("[White \"W\"]");
  const auto black_pos = normalized.find("[Black \"B\"]");
  const auto result_pos = normalized.find("[Result \"*\"]");

  ASSERT_NE(event_pos, std::string::npos);
  ASSERT_NE(white_pos, std::string::npos);
  ASSERT_NE(black_pos, std::string::npos);
  ASSERT_NE(result_pos, std::string::npos);
  EXPECT_LT(event_pos, white_pos);
  EXPECT_LT(white_pos, black_pos);
  EXPECT_LT(black_pos, result_pos);

  EXPECT_NE(normalized.find("$1"), std::string::npos);
  EXPECT_NE(normalized.find("$2"), std::string::npos);
  EXPECT_NE(normalized.find("{king side}"), std::string::npos);
}

TEST_F(TranslationUnitTest, UCIProtocolAdapterHandlesPositionAndGo)
{
  ChessProtocolAdapter adapter;
  ChessGameHandler handler;
  ChessEngine engine;

  const auto intro = adapter.handle_uci_command("uci", handler, engine);
  ASSERT_GE(intro.responses.size(), 3u);
  EXPECT_EQ(intro.responses.back(), "uciok");

  adapter.handle_uci_command("position startpos moves e2e4 e7e5", handler, engine);
  const auto piece = handler.get_current_grid().get_piece(Position(4, 3));
  ASSERT_TRUE(piece.has_value());
  EXPECT_EQ(piece->type, PieceType::PAWN);
  EXPECT_EQ(piece->color, Color::WHITE);

  const auto bestmove = adapter.handle_uci_command("go depth 1", handler, engine);
  ASSERT_EQ(bestmove.responses.size(), 1u);
  EXPECT_EQ(bestmove.responses[0].substr(0, 9), "bestmove ");
}

TEST_F(TranslationUnitTest, XBoardProtocolAdapterHandlesFeaturesForceAndGo)
{
  ChessProtocolAdapter adapter;
  ChessGameHandler handler;
  ChessEngine engine;
  ProtocolAdapterState state;

  const auto features = adapter.handle_xboard_command("protover 2", handler, engine, state);
  ASSERT_EQ(features.responses.size(), 1u);
  EXPECT_NE(features.responses[0].find("feature"), std::string::npos);

  adapter.handle_xboard_command("new", handler, engine, state);
  adapter.handle_xboard_command("force", handler, engine, state);
  const auto usermove = adapter.handle_xboard_command("usermove e2e4", handler, engine, state);
  EXPECT_TRUE(usermove.responses.empty());

  const auto pawn = handler.get_current_grid().get_piece(Position(4, 3));
  ASSERT_TRUE(pawn.has_value());
  EXPECT_EQ(pawn->type, PieceType::PAWN);
  EXPECT_EQ(pawn->color, Color::WHITE);

  const auto engine_move = adapter.handle_xboard_command("go", handler, engine, state);
  ASSERT_EQ(engine_move.responses.size(), 1u);
  EXPECT_EQ(engine_move.responses[0].substr(0, 5), "move ");
}
