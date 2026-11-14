#include "execution_budget.h"
#include "sqlite_game_application.h"
#include <gtest/gtest.h>
#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>

namespace
{
  using namespace Chess::Backend;
  using Milliseconds = std::chrono::milliseconds;
  using TimePoint = std::chrono::steady_clock::time_point;

  static_assert(!std::is_copy_constructible_v<ExecutionBudget>);
  static_assert(!std::is_move_constructible_v<ExecutionBudget>);
  static_assert(std::is_nothrow_destructible_v<ExecutionBudget>);

  TEST(ExecutionBudgetTest, DisabledOutsideScopeAndRestoredAfterUnwinding)
  {
    EXPECT_FALSE(ExecutionBudget::expired());
    EXPECT_NO_THROW(ExecutionBudget::checkpoint());
    TimePoint now{};
    EXPECT_THROW(
      {
        ExecutionBudget budget(Milliseconds(1), [&now] { return now; });
        now += Milliseconds(1);
        EXPECT_TRUE(ExecutionBudget::expired());
        ExecutionBudget::checkpoint();
      }, ExecutionDeadlineExceeded);
    EXPECT_FALSE(ExecutionBudget::expired());
    EXPECT_NO_THROW(ExecutionBudget::checkpoint());
  }

  TEST(ExecutionBudgetTest, ValidatesDurationWithoutReplacingActiveScope)
  {
    TimePoint now{};
    ExecutionBudget outer(Milliseconds(10), [&now] { return now; });
    for (const auto invalid : {Milliseconds(-1), Milliseconds(0), Milliseconds(60001), Milliseconds::max()})
    {
      EXPECT_THROW(ExecutionBudget rejected(invalid), std::invalid_argument);
    }
    EXPECT_NO_THROW(ExecutionBudget minimum(Milliseconds(1)));
    EXPECT_NO_THROW(ExecutionBudget maximum(Milliseconds(60000)));
    EXPECT_NO_THROW(ExecutionBudget nested(Milliseconds(1), []() -> TimePoint
      { throw std::runtime_error("unused nested clock"); }));
    now += Milliseconds(10);
    EXPECT_TRUE(ExecutionBudget::expired());
  }

  TEST(ExecutionBudgetTest, ClockFailureDoesNotEnableBudget)
  {
    EXPECT_THROW(ExecutionBudget budget(Milliseconds(1), []() -> TimePoint
      { throw std::runtime_error("clock failure"); }), std::runtime_error);
    EXPECT_FALSE(ExecutionBudget::expired());
  }

  TEST(ExecutionBudgetTest, RealClockIsAvailableWithoutInjection)
  {
    ExecutionBudget budget(Milliseconds(60000));
    EXPECT_NO_THROW(ExecutionBudget::checkpoint());
  }

  TEST(ExecutionBudgetTest, CheckpointStopsBoundedLoopAtExactDeadline)
  {
    TimePoint now{};
    ExecutionBudget budget(Milliseconds(3), [&now] { return now; });
    unsigned completed = 0;
    EXPECT_THROW(
      {
        for (unsigned iteration = 0; iteration < 10; ++iteration)
        {
          ExecutionBudget::checkpoint();
          ++completed;
          now += Milliseconds(1);
        }
      }, ExecutionDeadlineExceeded);
    EXPECT_EQ(completed, 3u);
    EXPECT_STREQ(ExecutionDeadlineExceeded{}.what(), "The request execution deadline was exceeded.");
  }

  TEST(ExecutionBudgetTest, ShorterNestedScopeRestoresAncestorDeadline)
  {
    TimePoint now{};
    ExecutionBudget outer(Milliseconds(10), [&now] { return now; });
    now += Milliseconds(2);
    EXPECT_THROW(
      {
        ExecutionBudget inner(Milliseconds(1));
        now += Milliseconds(1);
        ExecutionBudget::checkpoint();
      }, ExecutionDeadlineExceeded);
    EXPECT_FALSE(ExecutionBudget::expired());
    now = TimePoint{} + Milliseconds(10);
    EXPECT_TRUE(ExecutionBudget::expired());
  }

  TEST(ExecutionBudgetTest, LongerNestedScopeCannotExtendAncestorOrReplaceClock)
  {
    TimePoint now{};
    ExecutionBudget outer(Milliseconds(3), [&now] { return now; });
    now += Milliseconds(2);
    {
      ExecutionBudget inner(Milliseconds(60000), [] { return TimePoint{}; });
      now += Milliseconds(1);
      EXPECT_THROW(ExecutionBudget::checkpoint(), ExecutionDeadlineExceeded);
    }
    EXPECT_TRUE(ExecutionBudget::expired());
    {
      ExecutionBudget inner(Milliseconds(60000));
      EXPECT_TRUE(ExecutionBudget::expired());
    }
    EXPECT_TRUE(ExecutionBudget::expired());
  }

  TEST(ExecutionBudgetTest, ThreadsHaveIndependentScopesAndClocks)
  {
    TimePoint now{};
    ExecutionBudget outer(Milliseconds(1), [&now] { return now; });
    now += Milliseconds(1);
    std::thread worker([]
    {
      EXPECT_FALSE(ExecutionBudget::expired());
      TimePoint worker_now{};
      {
        ExecutionBudget budget(Milliseconds(1), [&worker_now] { return worker_now; });
        EXPECT_FALSE(ExecutionBudget::expired());
        worker_now += Milliseconds(1);
        EXPECT_THROW(ExecutionBudget::checkpoint(), ExecutionDeadlineExceeded);
      }
      EXPECT_FALSE(ExecutionBudget::expired());
    });
    worker.join();
    EXPECT_TRUE(ExecutionBudget::expired());
  }

  class ExpireAfterRetryInsert final
  {
    public:
      ExpireAfterRetryInsert(sqlite3* database, TimePoint& now)
        : database_(database), now_(now)
      {
        sqlite3_update_hook(database_, &observe, this);
      }

      ~ExpireAfterRetryInsert()
      {
        sqlite3_update_hook(database_, nullptr, nullptr);
      }

      ExpireAfterRetryInsert(const ExpireAfterRetryInsert&) = delete;
      ExpireAfterRetryInsert& operator=(const ExpireAfterRetryInsert&) = delete;

      unsigned games = 0;
      unsigned moves = 0;
      unsigned retries = 0;

    private:
      static void observe(void* context, int operation, const char*, const char* table, sqlite3_int64) noexcept
      {
        auto& self = *static_cast<ExpireAfterRetryInsert*>(context);
        if (table == nullptr)
        {
          return;
        }
        const std::string_view name(table);
        if (name == "games")
        {
          ++self.games;
        }
        if (name == "game_moves")
        {
          ++self.moves;
        }
        if (name == "idempotency_records" && operation == SQLITE_INSERT)
        {
          ++self.retries;
          self.now_ += Milliseconds(1);
        }
      }

      sqlite3* database_;
      TimePoint& now_;
  };

  class ExecutionBudgetSqliteTest : public ::testing::Test
  {
    protected:
      void SetUp() override
      {
        ASSERT_TRUE(database_.is_ready());
      }

      sqlite3_int64 count(const char* sql)
      {
        sqlite3_stmt* statement = nullptr;
        EXPECT_EQ(sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr), SQLITE_OK);
        if (!statement)
        {
          return -1;
        }
        EXPECT_EQ(sqlite3_step(statement), SQLITE_ROW);
        const auto result = sqlite3_column_int64(statement, 0);
        EXPECT_EQ(sqlite3_finalize(statement), SQLITE_OK);
        return result;
      }

      SqliteDatabase database_{":memory:"};
      SqliteGameApplication application_{database_};
      Principal white_{"budget-white"};
      Principal black_{"budget-black"};
  };

  class ReplayTrace final
  {
    public:
      explicit ReplayTrace(sqlite3* database) : database_(database)
      {
        sqlite3_trace_v2(database_, SQLITE_TRACE_STMT | SQLITE_TRACE_ROW, &observe, this);
      }

      ~ReplayTrace()
      {
        sqlite3_trace_v2(database_, 0, nullptr, nullptr);
      }

      ReplayTrace(const ReplayTrace&) = delete;
      ReplayTrace& operator=(const ReplayTrace&) = delete;

      bool started = false;
      unsigned rows = 0;

    private:
      static int observe(unsigned event, void* context, void* statement, void*) noexcept
      {
        const char* sql = sqlite3_sql(static_cast<sqlite3_stmt*>(statement));
        if (sql != nullptr && std::string_view(sql).find("SELECT uci, ply FROM game_moves") == 0)
        {
          auto& self = *static_cast<ReplayTrace*>(context);
          if (event == SQLITE_TRACE_STMT)
          {
            self.started = true;
          }
          if (event == SQLITE_TRACE_ROW)
          {
            ++self.rows;
          }
        }
        return 0;
      }

      sqlite3* database_;
  };

  TEST_F(ExecutionBudgetSqliteTest, ExpiredBudgetRejectsEveryApplicationEntry)
  {
    TimePoint now{};
    ExecutionBudget budget(Milliseconds(1), [&now] { return now; });
    now += Milliseconds(1);
    EXPECT_THROW(application_.is_ready(), ExecutionDeadlineExceeded);
    EXPECT_THROW(application_.create_game(white_, {}, "budget-create-key"), ExecutionDeadlineExceeded);
    EXPECT_THROW(application_.get_game(white_, "missing"), ExecutionDeadlineExceeded);
    EXPECT_THROW(application_.join_game(black_, "missing", 1, "budget-join-key-1"), ExecutionDeadlineExceeded);
    EXPECT_THROW(application_.get_legal_moves(white_, "missing"), ExecutionDeadlineExceeded);
    EXPECT_THROW(application_.get_move_history(white_, "missing", 0, 10), ExecutionDeadlineExceeded);
    EXPECT_THROW(application_.submit_move(white_, "missing", {"e2e4", 1}, "budget-move-key-1"), ExecutionDeadlineExceeded);
    EXPECT_THROW(application_.resign(white_, "missing", 1, "budget-resign-key"), ExecutionDeadlineExceeded);
    EXPECT_THROW(application_.offer_draw(white_, "missing", 1, "budget-offer-key-1"), ExecutionDeadlineExceeded);
    EXPECT_THROW(application_.accept_draw(white_, "missing", 1, "budget-accept-key"), ExecutionDeadlineExceeded);
  }

  TEST_F(ExecutionBudgetSqliteTest, CreateRollsBackGameAndRetryWhenDeadlineExpiresBeforeCommit)
  {
    TimePoint now{};
    {
      ExpireAfterRetryInsert writes(database_.handle(), now);
      {
        ExecutionBudget budget(Milliseconds(1), [&now] { return now; });
        EXPECT_THROW(application_.create_game(white_, {}, "budget-create-key"), ExecutionDeadlineExceeded);
      }
      EXPECT_EQ(writes.games, 1u);
      EXPECT_EQ(writes.retries, 1u);
    }
    EXPECT_NE(sqlite3_get_autocommit(database_.handle()), 0);
    EXPECT_EQ(count("SELECT count(*) FROM games"), 0);
    EXPECT_EQ(count("SELECT count(*) FROM idempotency_records"), 0);
    EXPECT_TRUE(std::holds_alternative<GameSnapshot>(application_.create_game(white_, {}, "budget-create-key")));
  }

  TEST_F(ExecutionBudgetSqliteTest, MoveRollsBackHistoryVersionAndRetryBeforeCommit)
  {
    const auto created = application_.create_game(white_, {}, "budget-create-key");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(created));
    const auto game_id = std::get<GameSnapshot>(created).game_id;
    const auto joined = application_.join_game(black_, game_id, 1, "budget-join-key-1");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(joined));
    const auto before = std::get<GameSnapshot>(joined);
    TimePoint now{};
    {
      ExpireAfterRetryInsert writes(database_.handle(), now);
      {
        ExecutionBudget budget(Milliseconds(1), [&now] { return now; });
        EXPECT_THROW(application_.submit_move(white_, game_id, {"e2e4", before.version}, "budget-move-key-1"),
                     ExecutionDeadlineExceeded);
      }
      EXPECT_EQ(writes.games, 1u);
      EXPECT_EQ(writes.moves, 1u);
      EXPECT_EQ(writes.retries, 1u);
    }
    EXPECT_NE(sqlite3_get_autocommit(database_.handle()), 0);
    EXPECT_EQ(count("SELECT count(*) FROM game_moves"), 0);
    EXPECT_EQ(count("SELECT count(*) FROM idempotency_records"), 2);
    const auto loaded = application_.get_game(white_, game_id);
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(loaded));
    EXPECT_EQ(std::get<GameSnapshot>(loaded).version, before.version);
    EXPECT_EQ(std::get<GameSnapshot>(loaded).fen, before.fen);
    EXPECT_TRUE(std::holds_alternative<GameSnapshot>(
      application_.submit_move(white_, game_id, {"e2e4", before.version}, "budget-move-key-1")));
  }

  TEST_F(ExecutionBudgetSqliteTest, ReplayTimesOutInsideFirstMovesLegalMoveLoop)
  {
    const auto created = application_.create_game(white_, {}, "budget-create-key");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(created));
    const auto game_id = std::get<GameSnapshot>(created).game_id;
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(
      application_.join_game(black_, game_id, 1, "budget-join-key-1")));
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(
      application_.submit_move(white_, game_id, {"e2e4", 2}, "budget-move-key-1")));
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(
      application_.submit_move(black_, game_id, {"e7e5", 3}, "budget-move-key-2")));
    {
      ReplayTrace trace(database_.handle());
      unsigned checkpoints = 0;
      {
        ExecutionBudget budget(Milliseconds(1), [&trace, &checkpoints]
        {
          if (trace.started && ++checkpoints >= 2)
          {
            return TimePoint{} + Milliseconds(1);
          }
          return TimePoint{};
        });
        EXPECT_THROW(application_.get_game(white_, game_id), ExecutionDeadlineExceeded);
      }
      EXPECT_TRUE(trace.started);
      EXPECT_EQ(trace.rows, 1u);
      EXPECT_EQ(checkpoints, 2u);
    }
    EXPECT_NE(sqlite3_get_autocommit(database_.handle()), 0);
    EXPECT_EQ(count("SELECT count(*) FROM game_moves"), 2);
    EXPECT_TRUE(std::holds_alternative<GameSnapshot>(application_.get_game(white_, game_id)));
  }
}