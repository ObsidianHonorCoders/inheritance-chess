#include "sqlite_game_application.h"
#include <gtest/gtest.h>
#include <atomic>
#include <barrier>
#include <chrono>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace
{
  using namespace Chess::Backend;

  class SqliteGameOperationsTest : public ::testing::Test
  {
    protected:
      void SetUp() override
      {
        static std::atomic<unsigned> sequence{0};
        path_ = std::filesystem::temp_directory_path()
                / ("ichess-operations-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
                   + "-" + std::to_string(sequence.fetch_add(1)) + ".db");
        reopen();
      }

      void TearDown() override
      {
        application_.reset();
        database_.reset();
        std::error_code cleanup_error;
        std::filesystem::remove(path_, cleanup_error);
        std::filesystem::remove(path_.string() + "-wal", cleanup_error);
        std::filesystem::remove(path_.string() + "-shm", cleanup_error);
      }

      void reopen()
      {
        application_.reset();
        database_.reset();
        database_ = std::make_unique<SqliteDatabase>(path_.string());
        ASSERT_TRUE(database_->is_ready());
        application_ = std::make_unique<SqliteGameApplication>(*database_);
      }

      std::string key()
      {
        return "operation-key-00000000-" + std::to_string(++key_sequence_);
      }

      GameSnapshot success(Result<GameSnapshot> result)
      {
        if (const auto* failure = std::get_if<ApplicationError>(&result))
        {
          ADD_FAILURE() << failure->status << " " << failure->code;
          return {};
        }
        return std::get<GameSnapshot>(std::move(result));
      }

      template <typename Value>
      void expect_error(const Result<Value>& result, int status)
      {
        ASSERT_TRUE(std::holds_alternative<ApplicationError>(result));
        EXPECT_EQ(std::get<ApplicationError>(result).status, status);
      }

      GameSnapshot create(const std::optional<std::string>& fen = std::nullopt)
      {
        CreateGameCommand command;
        command.initial_fen = fen;
        return success(application_->create_game(white_, command, key()));
      }

      GameSnapshot joined(const std::optional<std::string>& fen = std::nullopt)
      {
        const auto game = create(fen);
        return success(application_->join_game(black_, game.game_id, game.version, key()));
      }

      GameSnapshot move(const GameSnapshot& game, const std::string& uci)
      {
        const auto& principal = game.side_to_move == "white" ? white_ : black_;
        return success(application_->submit_move(principal, game.game_id, MoveCommand{uci, game.version}, key()));
      }

      void execute(const char* sql)
      {
        ASSERT_EQ(sqlite3_exec(database_->handle(), sql, nullptr, nullptr, nullptr), SQLITE_OK);
      }

      sqlite3_int64 integer(const char* sql)
      {
        sqlite3_stmt* statement = nullptr;
        EXPECT_EQ(sqlite3_prepare_v2(database_->handle(), sql, -1, &statement, nullptr), SQLITE_OK);
        if (statement == nullptr)
        {
          return -1;
        }
        EXPECT_EQ(sqlite3_step(statement), SQLITE_ROW);
        const auto value = sqlite3_column_int64(statement, 0);
        EXPECT_EQ(sqlite3_step(statement), SQLITE_DONE);
        sqlite3_finalize(statement);
        return value;
      }

      std::string text(const char* sql)
      {
        sqlite3_stmt* statement = nullptr;
        EXPECT_EQ(sqlite3_prepare_v2(database_->handle(), sql, -1, &statement, nullptr), SQLITE_OK);
        if (statement == nullptr)
        {
          return {};
        }
        EXPECT_EQ(sqlite3_step(statement), SQLITE_ROW);
        const auto* raw = sqlite3_column_text(statement, 0);
        const std::string value = raw ? reinterpret_cast<const char*>(raw) : "";
        EXPECT_EQ(sqlite3_step(statement), SQLITE_DONE);
        sqlite3_finalize(statement);
        return value;
      }

      static int reject_retry_insert(void*, int action, const char* table, const char*, const char*, const char*)
      {
        return action == SQLITE_INSERT && table != nullptr && std::string_view(table) == "idempotency_records"
                 ? SQLITE_DENY : SQLITE_OK;
      }

      static int reject_game_update(void*, int action, const char* table, const char*, const char*, const char*)
      {
        return action == SQLITE_UPDATE && table != nullptr && std::string_view(table) == "games"
                 ? SQLITE_DENY : SQLITE_OK;
      }

      std::filesystem::path path_;
      std::unique_ptr<SqliteDatabase> database_;
      std::unique_ptr<SqliteGameApplication> application_;
      Principal white_{"white-player"};
      Principal black_{"black-player"};
      Principal outsider_{"outsider"};
      unsigned key_sequence_ = 0;
  };

  TEST_F(SqliteGameOperationsTest, JoinClaimsOnlyEmptyBlackSeatAndHidesOtherReads)
  {
    const auto game = create();
    expect_error(application_->get_game(outsider_, game.game_id), 404);
    expect_error(application_->get_legal_moves(outsider_, game.game_id), 404);
    expect_error(application_->get_move_history(outsider_, game.game_id, 0, 100), 404);
    expect_error(application_->join_game(white_, game.game_id, game.version, key()), 409);
    const auto stale = application_->join_game(black_, game.game_id, 0, key());
    expect_error(stale, 412);
    EXPECT_EQ(std::get<ApplicationError>(stale).current_version, game.version);
    const auto claimed = success(application_->join_game(black_, game.game_id, game.version, key()));
    EXPECT_EQ(claimed.viewer_seat, "black");
    EXPECT_EQ(claimed.version, game.version + 1);
    EXPECT_TRUE(claimed.black_player.has_value());
    expect_error(application_->join_game(outsider_, game.game_id, claimed.version, key()), 409);
    expect_error(application_->join_game(black_, game.game_id, claimed.version, key()), 409);
    expect_error(application_->join_game(outsider_, "missing", 1, key()), 404);
    EXPECT_EQ(success(application_->get_game(white_, game.game_id)).viewer_seat, "white");
  }

  TEST_F(SqliteGameOperationsTest, JoinRetryPrecedesVersionAndReauthorizes)
  {
    const auto game = create();
    const auto retry_key = key();
    const auto claimed = success(application_->join_game(black_, game.game_id, 1, retry_key));
    const auto advanced = move(claimed, "e2e4");
    EXPECT_EQ(advanced.version, 3u);
    const auto replay = success(application_->join_game(black_, game.game_id, 1, retry_key));
    EXPECT_EQ(replay.version, 2u);
    EXPECT_EQ(replay.fen, claimed.fen);
    EXPECT_EQ(replay.server_time, claimed.server_time);
    expect_error(application_->join_game(black_, game.game_id, 2, retry_key), 409);
    execute("UPDATE games SET black_principal = 'replacement'");
    expect_error(application_->join_game(black_, game.game_id, 1, retry_key), 404);
  }

  TEST_F(SqliteGameOperationsTest, LegalMovesAndHistoryTrackVersionSanAndPagination)
  {
    auto game = joined();
    auto legal = application_->get_legal_moves(white_, game.game_id);
    ASSERT_TRUE(std::holds_alternative<LegalMoveList>(legal));
    EXPECT_EQ(std::get<LegalMoveList>(legal).version, 2u);
    EXPECT_EQ(std::get<LegalMoveList>(legal).legal_moves.size(), 20u);
    auto empty = application_->get_move_history(white_, game.game_id, 0, 200);
    ASSERT_TRUE(std::holds_alternative<MovePage>(empty));
    EXPECT_TRUE(std::get<MovePage>(empty).moves.empty());
    EXPECT_FALSE(std::get<MovePage>(empty).next_after_ply);
    game = move(game, "e2e4");
    game = move(game, "e7e5");
    game = move(game, "g1f3");
    const auto first = application_->get_move_history(black_, game.game_id, 0, 2);
    ASSERT_TRUE(std::holds_alternative<MovePage>(first));
    const auto& page = std::get<MovePage>(first);
    ASSERT_EQ(page.moves.size(), 2u);
    EXPECT_EQ(page.moves[0].ply, 1u);
    EXPECT_EQ(page.moves[0].uci, "e2e4");
    EXPECT_EQ(page.moves[0].san, "e4");
    EXPECT_EQ(page.next_after_ply, 2u);
    const auto second = application_->get_move_history(white_, game.game_id, 2, 2);
    ASSERT_TRUE(std::holds_alternative<MovePage>(second));
    ASSERT_EQ(std::get<MovePage>(second).moves.size(), 1u);
    EXPECT_EQ(std::get<MovePage>(second).moves[0].san, "Nf3");
    EXPECT_FALSE(std::get<MovePage>(second).next_after_ply);
    const auto tail = application_->get_move_history(white_, game.game_id, 3, 1);
    ASSERT_TRUE(std::holds_alternative<MovePage>(tail));
    EXPECT_TRUE(std::get<MovePage>(tail).moves.empty());
    expect_error(application_->get_move_history(white_, game.game_id, 0, 0), 400);
    expect_error(application_->get_move_history(white_, game.game_id, 0, 201), 400);
    expect_error(application_->get_move_history(white_, game.game_id, (std::numeric_limits<std::uint64_t>::max)(), 1), 400);
    reopen();
    const auto restored = success(application_->get_game(black_, game.game_id));
    EXPECT_EQ(restored.fen, game.fen);
    EXPECT_EQ(restored.version, 5u);
    EXPECT_EQ(restored.move_count, 3u);
  }

  TEST_F(SqliteGameOperationsTest, MoveChecksSeatTurnVersionShapeAndLegalityWithoutWrites)
  {
    const auto waiting = create();
    expect_error(application_->submit_move(white_, waiting.game_id, {"e2e4", 1}, key()), 409);
    const auto game = success(application_->join_game(black_, waiting.game_id, 1, key()));
    expect_error(application_->submit_move(outsider_, game.game_id, {"e2e4", 2}, key()), 404);
    expect_error(application_->submit_move(black_, game.game_id, {"e7e5", 2}, key()), 409);
    expect_error(application_->submit_move(white_, game.game_id, {"E2E4", 2}, key()), 400);
    expect_error(application_->submit_move(white_, game.game_id, {"e2e5", 2}, key()), 422);
    const auto stale = application_->submit_move(white_, game.game_id, {"e2e4", 1}, key());
    expect_error(stale, 412);
    EXPECT_EQ(std::get<ApplicationError>(stale).current_version, 2u);
    EXPECT_EQ(integer("SELECT count(*) FROM game_moves"), 0);
    EXPECT_EQ(success(application_->get_game(white_, game.game_id)).version, 2u);
  }

  TEST_F(SqliteGameOperationsTest, MoveRetryReturnsOriginalResponseAndRejectsAlteredPayloadOrVersion)
  {
    const auto game = joined();
    const auto retry_key = key();
    const auto first = success(application_->submit_move(white_, game.game_id, {"e2e4", 2}, retry_key));
    move(first, "e7e5");
    const auto replay = success(application_->submit_move(white_, game.game_id, {"e2e4", 2}, retry_key));
    EXPECT_EQ(replay.version, 3u);
    EXPECT_EQ(replay.fen, first.fen);
    EXPECT_EQ(replay.server_time, first.server_time);
    expect_error(application_->submit_move(white_, game.game_id, {"d2d4", 2}, retry_key), 409);
    expect_error(application_->submit_move(white_, game.game_id, {"e2e4", 4}, retry_key), 409);
    EXPECT_EQ(integer("SELECT count(*) FROM game_moves"), 2);
    execute("UPDATE games SET white_principal = 'replacement'");
    expect_error(application_->submit_move(white_, game.game_id, {"e2e4", 2}, retry_key), 404);
  }

  TEST_F(SqliteGameOperationsTest, IdempotencyIsScopedToPrincipalOperationAndResource)
  {
    const std::string shared = "shared-key-0123456789";
    const auto first = joined();
    const auto second = joined();
    const auto first_move = success(application_->submit_move(white_, first.game_id, {"e2e4", 2}, shared));
    const auto second_move = success(application_->submit_move(white_, second.game_id, {"d2d4", 2}, shared));
    EXPECT_EQ(first_move.version, 3u);
    EXPECT_EQ(second_move.version, 3u);
    const auto black_move = success(application_->submit_move(black_, first.game_id, {"e7e5", 3}, shared));
    const auto offer = success(application_->offer_draw(white_, first.game_id, black_move.version, shared));
    const auto resigned = success(application_->resign(white_, first.game_id, offer.version, shared));
    EXPECT_EQ(resigned.status, "white_resigned");
    EXPECT_EQ(resigned.version, 6u);
  }

  TEST_F(SqliteGameOperationsTest, ExpiredMutationRetryUsesFreshVersionPreconditionAndCanReuseKey)
  {
    auto game = joined();
    const auto retry_key = key();
    game = success(application_->submit_move(white_, game.game_id, {"e2e4", 2}, retry_key));
    execute("UPDATE idempotency_records SET created_at = '2000-01-01T00:00:00Z'");
    expect_error(application_->submit_move(white_, game.game_id, {"e2e4", 2}, retry_key), 412);
    game = move(game, "e7e5");
    game = success(application_->submit_move(white_, game.game_id, {"g1f3", game.version}, retry_key));
    EXPECT_EQ(game.move_count, 3u);
    EXPECT_EQ(game.version, 5u);
  }

  TEST_F(SqliteGameOperationsTest, ResignationIsDurableTerminalAndRetryable)
  {
    const auto game = joined();
    const auto retry_key = key();
    const auto resigned = success(application_->resign(black_, game.game_id, 2, retry_key));
    EXPECT_EQ(resigned.status, "black_resigned");
    EXPECT_EQ(resigned.version, 3u);
    EXPECT_TRUE(resigned.legal_moves.empty());
    EXPECT_EQ(text("SELECT status FROM games"), "black_resigned");
    reopen();
    EXPECT_EQ(success(application_->get_game(white_, game.game_id)).status, "black_resigned");
    EXPECT_EQ(success(application_->resign(black_, game.game_id, 2, retry_key)).version, 3u);
    expect_error(application_->resign(white_, game.game_id, 3, key()), 409);
    expect_error(application_->submit_move(white_, game.game_id, {"e2e4", 3}, key()), 409);
    expect_error(application_->offer_draw(white_, game.game_id, 3, key()), 409);
    expect_error(application_->accept_draw(white_, game.game_id, 3, key()), 409);
    const auto legal = application_->get_legal_moves(white_, game.game_id);
    ASSERT_TRUE(std::holds_alternative<LegalMoveList>(legal));
    EXPECT_TRUE(std::get<LegalMoveList>(legal).legal_moves.empty());
  }

  TEST_F(SqliteGameOperationsTest, DrawOfferPersistsAcrossRestartAndOnlyOpponentCanAccept)
  {
    const auto game = joined();
    expect_error(application_->accept_draw(white_, game.game_id, 2, key()), 409);
    const auto offer_key = key();
    const auto offered = success(application_->offer_draw(white_, game.game_id, 2, offer_key));
    EXPECT_EQ(offered.version, 3u);
    EXPECT_EQ(offered.status, "ongoing");
    EXPECT_EQ(text("SELECT draw_offer_by FROM games"), "white");
    reopen();
    EXPECT_EQ(success(application_->offer_draw(white_, game.game_id, 2, offer_key)).version, 3u);
    expect_error(application_->accept_draw(white_, game.game_id, 3, key()), 409);
    expect_error(application_->offer_draw(black_, game.game_id, 3, key()), 409);
    expect_error(application_->accept_draw(black_, game.game_id, 2, key()), 412);
    const auto accept_key = key();
    const auto accepted = success(application_->accept_draw(black_, game.game_id, 3, accept_key));
    EXPECT_EQ(accepted.version, 4u);
    EXPECT_EQ(accepted.status, "draw_by_agreement");
    EXPECT_TRUE(accepted.legal_moves.empty());
    EXPECT_EQ(integer("SELECT draw_offer_by IS NULL FROM games"), 1);
    reopen();
    EXPECT_EQ(success(application_->get_game(white_, game.game_id)).status, "draw_by_agreement");
    EXPECT_EQ(success(application_->accept_draw(black_, game.game_id, 3, accept_key)).version, 4u);
    expect_error(application_->accept_draw(black_, game.game_id, 4, key()), 409);
  }

  TEST_F(SqliteGameOperationsTest, MovesClearPendingDrawOfferAndBlackMayOffer)
  {
    auto game = joined();
    game = success(application_->offer_draw(black_, game.game_id, game.version, key()));
    EXPECT_EQ(text("SELECT draw_offer_by FROM games"), "black");
    expect_error(application_->accept_draw(black_, game.game_id, game.version, key()), 409);
    game = move(game, "e2e4");
    EXPECT_EQ(integer("SELECT draw_offer_by IS NULL FROM games"), 1);
    expect_error(application_->accept_draw(white_, game.game_id, game.version, key()), 409);
    game = success(application_->offer_draw(black_, game.game_id, game.version, key()));
    game = success(application_->accept_draw(white_, game.game_id, game.version, key()));
    EXPECT_EQ(game.status, "draw_by_agreement");
  }

  TEST_F(SqliteGameOperationsTest, EveryLifecycleMutationHidesOutsidersAndRejectsStaleVersions)
  {
    const auto game = joined();
    expect_error(application_->resign(outsider_, game.game_id, 2, key()), 404);
    expect_error(application_->offer_draw(outsider_, game.game_id, 2, key()), 404);
    expect_error(application_->accept_draw(outsider_, game.game_id, 2, key()), 404);
    expect_error(application_->resign(white_, game.game_id, 1, key()), 412);
    expect_error(application_->offer_draw(white_, game.game_id, 1, key()), 412);
    expect_error(application_->accept_draw(white_, game.game_id, 1, key()), 412);
    EXPECT_EQ(success(application_->get_game(white_, game.game_id)).version, 2u);
  }

  TEST_F(SqliteGameOperationsTest, RestartReplayPreservesThreefoldRepetition)
  {
    auto game = joined();
    for (const std::string uci : {"g1f3", "g8f6", "f3g1", "f6g8"})
    {
      game = move(game, uci);
    }
    EXPECT_EQ(game.status, "ongoing");
    reopen();
    game = success(application_->get_game(white_, game.game_id));
    for (const std::string uci : {"g1f3", "g8f6", "f3g1", "f6g8"})
    {
      game = move(game, uci);
    }
    EXPECT_EQ(game.status, "draw_threefold_repetition");
    EXPECT_EQ(game.move_count, 8u);
    EXPECT_TRUE(game.legal_moves.empty());
    EXPECT_EQ(text("SELECT status FROM games"), "draw_threefold_repetition");
    reopen();
    const auto restored = success(application_->get_game(white_, game.game_id));
    EXPECT_EQ(restored.status, game.status);
    EXPECT_EQ(restored.fen, game.fen);
    expect_error(application_->submit_move(white_, game.game_id, {"e2e4", game.version}, key()), 409);
  }

  TEST_F(SqliteGameOperationsTest, PromotionPersistsUciSanAndRestoresAfterRestart)
  {
    auto game = joined("7k/P7/8/8/8/8/8/4K3 w - - 0 1");
    expect_error(application_->submit_move(white_, game.game_id, {"a7a8", game.version}, key()), 422);
    game = move(game, "a7a8q");
    EXPECT_EQ(game.fen.substr(0, 4), "Q6k/");
    EXPECT_EQ(text("SELECT uci FROM game_moves"), "a7a8q");
    EXPECT_EQ(text("SELECT san FROM game_moves"), "a8=Q+");
    reopen();
    EXPECT_EQ(success(application_->get_game(black_, game.game_id)).fen, game.fen);
  }

  TEST_F(SqliteGameOperationsTest, CastlingAndEnPassantUseReconstructedSpecialMoveFlags)
  {
    auto castling = joined("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    castling = move(castling, "e1g1");
    EXPECT_NE(castling.fen.find("R4RK1"), std::string::npos);
    reopen();
    EXPECT_EQ(success(application_->get_game(black_, castling.game_id)).fen, castling.fen);
    auto en_passant = joined("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2");
    en_passant = move(en_passant, "e5d6");
    reopen();
    EXPECT_EQ(success(application_->get_game(black_, en_passant.game_id)).fen, en_passant.fen);
    EXPECT_EQ(en_passant.move_count, 1u);
  }

  TEST_F(SqliteGameOperationsTest, CheckmatePersistsAndSuppressesLegalMoves)
  {
    auto game = joined();
    for (const std::string uci : {"f2f3", "e7e5", "g2g4", "d8h4"})
    {
      game = move(game, uci);
    }
    EXPECT_EQ(game.status, "checkmate");
    EXPECT_TRUE(game.legal_moves.empty());
    EXPECT_EQ(text("SELECT status FROM games"), "checkmate");
    reopen();
    EXPECT_EQ(success(application_->get_game(white_, game.game_id)).status, "checkmate");
  }

  TEST_F(SqliteGameOperationsTest, InitialTerminalStatusIsPersistedAndCannotBeJoined)
  {
    const auto game = create("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    EXPECT_EQ(game.status, "draw_insufficient_material");
    EXPECT_EQ(text("SELECT status FROM games"), game.status);
    EXPECT_TRUE(game.legal_moves.empty());
    expect_error(application_->join_game(black_, game.game_id, 1, key()), 409);
  }

  TEST_F(SqliteGameOperationsTest, IndependentConnectionsCannotCommitCompetingMovesAtSameVersion)
  {
    const auto game = joined();
    SqliteDatabase other_database(path_.string());
    ASSERT_TRUE(other_database.is_ready());
    SqliteGameApplication other(other_database);
    Result<GameSnapshot> first;
    Result<GameSnapshot> second;
    std::barrier start(3);
    std::thread first_thread([&] {
      start.arrive_and_wait();
      first = application_->submit_move(white_, game.game_id, {"e2e4", 2}, "race-first-0123456789");
    });
    std::thread second_thread([&] {
      start.arrive_and_wait();
      second = other.submit_move(white_, game.game_id, {"d2d4", 2}, "race-second-0123456789");
    });
    start.arrive_and_wait();
    first_thread.join();
    second_thread.join();
    ASSERT_NE(std::holds_alternative<GameSnapshot>(first), std::holds_alternative<GameSnapshot>(second));
    const auto& loser = std::holds_alternative<ApplicationError>(first) ? first : second;
    expect_error(loser, 412);
    EXPECT_EQ(std::get<ApplicationError>(loser).current_version, 3u);
    EXPECT_EQ(integer("SELECT count(*) FROM game_moves"), 1);
    EXPECT_EQ(success(application_->get_game(white_, game.game_id)).version, 3u);
  }

  TEST_F(SqliteGameOperationsTest, IndependentConnectionsReplayIdenticalConcurrentMoveOnlyOnce)
  {
    const auto game = joined();
    SqliteDatabase other_database(path_.string());
    ASSERT_TRUE(other_database.is_ready());
    SqliteGameApplication other(other_database);
    Result<GameSnapshot> first;
    Result<GameSnapshot> second;
    std::barrier start(3);
    std::thread first_thread([&] {
      start.arrive_and_wait();
      first = application_->submit_move(white_, game.game_id, {"e2e4", 2}, "race-shared-0123456789");
    });
    std::thread second_thread([&] {
      start.arrive_and_wait();
      second = other.submit_move(white_, game.game_id, {"e2e4", 2}, "race-shared-0123456789");
    });
    start.arrive_and_wait();
    first_thread.join();
    second_thread.join();
    const auto first_snapshot = success(first);
    const auto second_snapshot = success(second);
    EXPECT_EQ(first_snapshot.version, 3u);
    EXPECT_EQ(first_snapshot.fen, second_snapshot.fen);
    EXPECT_EQ(first_snapshot.server_time, second_snapshot.server_time);
    EXPECT_EQ(integer("SELECT count(*) FROM game_moves"), 1);
  }

  TEST_F(SqliteGameOperationsTest, IndependentConnectionsCannotClaimSameBlackSeat)
  {
    const auto game = create();
    SqliteDatabase other_database(path_.string());
    ASSERT_TRUE(other_database.is_ready());
    SqliteGameApplication other(other_database);
    Result<GameSnapshot> first;
    Result<GameSnapshot> second;
    std::barrier start(3);
    std::thread first_thread([&] {
      start.arrive_and_wait();
      first = application_->join_game(black_, game.game_id, 1, "join-first-0123456789");
    });
    std::thread second_thread([&] {
      start.arrive_and_wait();
      second = other.join_game(outsider_, game.game_id, 1, "join-second-0123456789");
    });
    start.arrive_and_wait();
    first_thread.join();
    second_thread.join();
    ASSERT_NE(std::holds_alternative<GameSnapshot>(first), std::holds_alternative<GameSnapshot>(second));
    expect_error(std::holds_alternative<ApplicationError>(first) ? first : second, 409);
    EXPECT_EQ(success(application_->get_game(white_, game.game_id)).version, 2u);
  }

  TEST_F(SqliteGameOperationsTest, FailedRetryRecordRollsBackMoveHistoryStateAndVersion)
  {
    const auto game = joined();
    const auto retry_key = key();
    ASSERT_EQ(sqlite3_set_authorizer(database_->handle(), reject_retry_insert, nullptr), SQLITE_OK);
    const auto failed = application_->submit_move(white_, game.game_id, {"e2e4", 2}, retry_key);
    ASSERT_EQ(sqlite3_set_authorizer(database_->handle(), nullptr, nullptr), SQLITE_OK);
    expect_error(failed, 500);
    EXPECT_EQ(integer("SELECT count(*) FROM game_moves"), 0);
    const auto unchanged = success(application_->get_game(white_, game.game_id));
    EXPECT_EQ(unchanged.version, game.version);
    EXPECT_EQ(unchanged.fen, game.fen);
    EXPECT_EQ(success(application_->submit_move(white_, game.game_id, {"e2e4", 2}, retry_key)).version, 3u);
  }

  TEST_F(SqliteGameOperationsTest, FailedStateUpdateRollsBackAlreadyAppendedMove)
  {
    const auto game = joined();
    ASSERT_EQ(sqlite3_set_authorizer(database_->handle(), reject_game_update, nullptr), SQLITE_OK);
    const auto failed = application_->submit_move(white_, game.game_id, {"e2e4", 2}, key());
    ASSERT_EQ(sqlite3_set_authorizer(database_->handle(), nullptr, nullptr), SQLITE_OK);
    expect_error(failed, 500);
    EXPECT_EQ(integer("SELECT count(*) FROM game_moves"), 0);
    EXPECT_EQ(success(application_->get_game(white_, game.game_id)).version, 2u);
  }

  TEST_F(SqliteGameOperationsTest, FailedRetryRecordRollsBackJoinAndLifecycleMutations)
  {
    auto game = create();
    ASSERT_EQ(sqlite3_set_authorizer(database_->handle(), reject_retry_insert, nullptr), SQLITE_OK);
    const auto join_failed = application_->join_game(black_, game.game_id, 1, key());
    ASSERT_EQ(sqlite3_set_authorizer(database_->handle(), nullptr, nullptr), SQLITE_OK);
    expect_error(join_failed, 500);
    EXPECT_FALSE(success(application_->get_game(white_, game.game_id)).black_player);
    game = success(application_->join_game(black_, game.game_id, 1, key()));
    ASSERT_EQ(sqlite3_set_authorizer(database_->handle(), reject_retry_insert, nullptr), SQLITE_OK);
    const auto offer_failed = application_->offer_draw(white_, game.game_id, 2, key());
    const auto resign_failed = application_->resign(white_, game.game_id, 2, key());
    ASSERT_EQ(sqlite3_set_authorizer(database_->handle(), nullptr, nullptr), SQLITE_OK);
    expect_error(offer_failed, 500);
    expect_error(resign_failed, 500);
    EXPECT_EQ(integer("SELECT draw_offer_by IS NULL FROM games"), 1);
    game = success(application_->offer_draw(white_, game.game_id, 2, key()));
    ASSERT_EQ(sqlite3_set_authorizer(database_->handle(), reject_retry_insert, nullptr), SQLITE_OK);
    const auto accept_failed = application_->accept_draw(black_, game.game_id, 3, key());
    ASSERT_EQ(sqlite3_set_authorizer(database_->handle(), nullptr, nullptr), SQLITE_OK);
    expect_error(accept_failed, 500);
    EXPECT_EQ(text("SELECT draw_offer_by FROM games"), "white");
    EXPECT_EQ(success(application_->get_game(white_, game.game_id)).status, "ongoing");
    EXPECT_EQ(success(application_->get_game(white_, game.game_id)).version, 3u);
  }

  TEST_F(SqliteGameOperationsTest, RejectsInconsistentCastlingAndEnPassantFenMetadata)
  {
    for (const std::string fen : {
      "4k3/8/8/8/8/8/8/4K3 w K - 0 1",
      "4k3/8/8/8/8/8/8/3K3R w K - 0 1",
      "4k3/8/8/8/8/8/8/r3K3 w Q - 0 1",
      "4k3/8/8/8/8/8/8/4K3 w k - 0 1",
      "4k3/8/8/8/8/8/8/4K3 w - d6 0 1",
      "4k3/8/8/3P4/8/8/8/4K3 w - d6 0 1",
      "4k3/3n4/8/3p4/8/8/8/4K3 w - d6 0 1",
      "4k3/8/3N4/3p4/8/8/8/4K3 w - d6 0 1",
      "4k3/8/8/3p4/8/8/8/4K3 w - d6 1 1",
      "4k3/8/8/8/3P4/8/8/4K3 w - d3 0 1",
      "4k3/8/8/8/3p4/8/8/4K3 b - d3 0 1"})
    {
      SCOPED_TRACE(fen);
      CreateGameCommand command;
      command.initial_fen = fen;
      const auto result = application_->create_game(white_, command, key());
      expect_error(result, 422);
      ASSERT_TRUE(std::holds_alternative<ApplicationError>(result));
      EXPECT_EQ(std::get<ApplicationError>(result).code, "invalid_fen");
    }
    EXPECT_EQ(integer("SELECT count(*) FROM games"), 0);
  }

  TEST_F(SqliteGameOperationsTest, ValidEnPassantDoesNotRequireAdjacentCapturingPawn)
  {
    EXPECT_EQ(create("4k3/8/8/3p4/8/8/8/4K3 w - d6 0 2").move_count, 0u);
    EXPECT_EQ(create("4k3/8/8/8/3P4/8/8/4K3 b - d3 0 2").move_count, 0u);
  }

  TEST_F(SqliteGameOperationsTest, SignedVersionLimitNeverWrapsOrBecomesReal)
  {
    const auto game = joined();
    execute("UPDATE games SET version = 9223372036854775807");
    const auto maximum = static_cast<std::uint64_t>((std::numeric_limits<sqlite3_int64>::max)());
    expect_error(application_->submit_move(white_, game.game_id, {"e2e4", maximum}, key()), 409);
    expect_error(application_->resign(white_, game.game_id, maximum, key()), 409);
    expect_error(application_->offer_draw(white_, game.game_id, maximum, key()), 409);
    const auto stale = application_->resign(white_, game.game_id, (std::numeric_limits<std::uint64_t>::max)(), key());
    expect_error(stale, 412);
    EXPECT_EQ(std::get<ApplicationError>(stale).current_version, maximum);
    EXPECT_EQ(integer("SELECT typeof(version) = 'integer' FROM games"), 1);
    EXPECT_EQ(integer("SELECT count(*) FROM game_moves"), 0);
    execute("UPDATE games SET version = 9223372036854775808.0");
    expect_error(application_->get_game(white_, game.game_id), 500);
  }

  TEST_F(SqliteGameOperationsTest, OversizedAndCorruptHistoryFailClosed)
  {
    const auto game = joined();
    execute("INSERT INTO game_moves(game_id, ply, uci, san) SELECT game_id, 4097, 'e2e4', 'e4' FROM games");
    const auto oversized = application_->get_game(white_, game.game_id);
    expect_error(oversized, 409);
    EXPECT_EQ(std::get<ApplicationError>(oversized).code, "history_limit");
    expect_error(application_->get_move_history(white_, game.game_id, 0, 100), 409);
    expect_error(application_->submit_move(white_, game.game_id, {"e2e4", 2}, key()), 409);
    execute("UPDATE game_moves SET ply = 2");
    expect_error(application_->get_game(white_, game.game_id), 500);
    execute("UPDATE game_moves SET ply = 1, uci = 'e2e5'");
    expect_error(application_->get_game(white_, game.game_id), 500);
    expect_error(application_->resign(white_, game.game_id, 2, key()), 500);
  }

  TEST_F(SqliteGameOperationsTest, InvalidPersistedStatusAndRetryJsonFailClosed)
  {
    const auto game = joined();
    execute("UPDATE games SET status = 'unexpected'");
    expect_error(application_->get_game(white_, game.game_id), 500);
    execute("UPDATE games SET status = 'ongoing'");
    const auto retry_key = key();
    success(application_->submit_move(white_, game.game_id, {"e2e4", 2}, retry_key));
    execute("UPDATE idempotency_records SET response_json = '{' WHERE operation LIKE 'move:%'");
    expect_error(application_->submit_move(white_, game.game_id, {"e2e4", 2}, retry_key), 500);
  }

  TEST_F(SqliteGameOperationsTest, MissingStorageAndInvalidMutationInputsReturnControlledErrors)
  {
    const auto game = joined();
    expect_error(application_->join_game(Principal{}, game.game_id, 2, key()), 400);
    expect_error(application_->resign(white_, game.game_id, 2, "short"), 400);
    expect_error(application_->submit_move(white_, game.game_id, {"e2e4qgarbage", 2}, key()), 400);
    SqliteDatabase missing((path_ / "missing" / "games.db").string());
    SqliteGameApplication unavailable(missing);
    expect_error(unavailable.join_game(black_, game.game_id, 2, key()), 503);
    expect_error(unavailable.get_legal_moves(white_, game.game_id), 503);
    expect_error(unavailable.get_move_history(white_, game.game_id, 0, 100), 503);
    expect_error(unavailable.submit_move(white_, game.game_id, {"e2e4", 2}, key()), 503);
    expect_error(unavailable.resign(white_, game.game_id, 2, key()), 503);
    expect_error(unavailable.offer_draw(white_, game.game_id, 2, key()), 503);
    expect_error(unavailable.accept_draw(white_, game.game_id, 2, key()), 503);
  }
}