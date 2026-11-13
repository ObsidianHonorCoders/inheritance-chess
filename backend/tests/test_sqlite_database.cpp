#include "sqlite_database.h"

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <utility>

namespace
{
  using Chess::Backend::SqliteDatabase;
  using Connection = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;
  using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

  Statement prepare(sqlite3* database, const char* sql)
  {
    sqlite3_stmt* statement = nullptr;
    const int result = sqlite3_prepare_v2(database, sql, -1, &statement, nullptr);
    EXPECT_EQ(result, SQLITE_OK) << sqlite3_errmsg(database);
    return Statement(statement, sqlite3_finalize);
  }

  std::string schema_sql(sqlite3* database)
  {
    auto statement = prepare(database,
      "SELECT sql FROM sqlite_schema WHERE sql IS NOT NULL AND name NOT LIKE 'sqlite_%' "
      "ORDER BY type DESC, name");
    std::string result;
    if (!statement)
    {
      return result;
    }
    int step = SQLITE_OK;
    while ((step = sqlite3_step(statement.get())) == SQLITE_ROW)
    {
      result += reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 0));
      result += ";\n";
    }
    EXPECT_EQ(step, SQLITE_DONE);
    return result;
  }

  int scalar(sqlite3* database, const char* sql)
  {
    auto statement = prepare(database, sql);
    if (!statement || sqlite3_step(statement.get()) != SQLITE_ROW)
    {
      ADD_FAILURE() << "Expected an integer row: " << sql;
      return -1;
    }
    return sqlite3_column_int(statement.get(), 0);
  }

  class SqliteDatabaseTest : public ::testing::Test
  {
    protected:
      void SetUp() override
      {
        static std::atomic<unsigned int> sequence{0};
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path()
          / ("ichess-schema-" + std::to_string(timestamp) + "-"
             + std::to_string(sequence.fetch_add(1)) + ".sqlite");
        SqliteDatabase reference(":memory:");
        ASSERT_TRUE(reference.is_ready());
        schema_ = schema_sql(reference.handle());
        ASSERT_FALSE(schema_.empty());
      }

      void TearDown() override
      {
        std::error_code error;
        std::filesystem::remove(path_, error);
        std::filesystem::remove(path_.string() + "-wal", error);
        std::filesystem::remove(path_.string() + "-shm", error);
      }

      Connection open() const
      {
        sqlite3* database = nullptr;
        const int result = sqlite3_open(path_.string().c_str(), &database);
        EXPECT_EQ(result, SQLITE_OK);
        return Connection(database, sqlite3_close);
      }

      void seed(const std::string& sql)
      {
        auto database = open();
        ASSERT_NE(database, nullptr);
        ASSERT_EQ(sqlite3_exec(database.get(), sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK)
          << sqlite3_errmsg(database.get());
      }

      void seed_valid()
      {
        seed(schema_ + "INSERT INTO schema_version VALUES(2);");
      }

      void expect_rejected_unchanged()
      {
        auto before = open();
        const auto original_schema = schema_sql(before.get());
        before.reset();
        SqliteDatabase database(path_.string());
        EXPECT_FALSE(database.is_ready());
        ASSERT_NE(database.handle(), nullptr);
        EXPECT_TRUE(sqlite3_get_autocommit(database.handle()));
        EXPECT_EQ(schema_sql(database.handle()), original_schema);
      }

      std::filesystem::path path_;
      std::string schema_;
  };

  TEST_F(SqliteDatabaseTest, FreshDatabaseAndReopenAreReadyWithoutProbeRows)
  {
    {
      SqliteDatabase database(path_.string());
      ASSERT_TRUE(database.is_ready());
      EXPECT_TRUE(sqlite3_get_autocommit(database.handle()));
      EXPECT_EQ(scalar(database.handle(), "SELECT count(*) FROM games"), 0);
      EXPECT_EQ(scalar(database.handle(), "SELECT count(*) FROM game_moves"), 0);
      EXPECT_EQ(scalar(database.handle(), "SELECT count(*) FROM schema_version"), 1);
    }
    SqliteDatabase reopened(path_.string());
    EXPECT_TRUE(reopened.is_ready());
  }

  TEST_F(SqliteDatabaseTest, ExistingDataSurvivesValidation)
  {
    seed_valid();
    seed("INSERT INTO games(game_id,white_principal,initial_fen,created_at,updated_at) "
         "VALUES('existing','white','fen','created','updated');"
         "INSERT INTO game_moves VALUES('existing',1,'e2e4','e4');");
    SqliteDatabase database(path_.string());
    ASSERT_TRUE(database.is_ready());
    EXPECT_EQ(scalar(database.handle(), "SELECT count(*) FROM games"), 1);
    EXPECT_EQ(scalar(database.handle(), "SELECT count(*) FROM game_moves"), 1);
    EXPECT_EQ(scalar(database.handle(), "SELECT version FROM games WHERE game_id='existing'"), 1);
  }

  class SqliteDatabaseMalformedSchemaTest
    : public SqliteDatabaseTest, public ::testing::WithParamInterface<std::pair<const char*, const char*>>
  {};

  TEST_P(SqliteDatabaseMalformedSchemaTest, FailsClosedAndRollsBack)
  {
    const auto& [original, replacement] = GetParam();
    const auto position = schema_.find(original);
    ASSERT_NE(position, std::string::npos);
    schema_.replace(position, std::string(original).size(), replacement);
    seed_valid();
    expect_rejected_unchanged();
    auto database = open();
    EXPECT_EQ(scalar(database.get(), "SELECT count(*) FROM games"), 0);
    EXPECT_EQ(scalar(database.get(), "SELECT count(*) FROM game_moves"), 0);
    EXPECT_EQ(scalar(database.get(), "SELECT count(*) FROM schema_version"), 1);
  }

  INSTANTIATE_TEST_SUITE_P(VersionOne, SqliteDatabaseMalformedSchemaTest, ::testing::Values(
    std::make_pair("white_principal TEXT NOT NULL", "white_principal TEXT"),
    std::make_pair("initial_fen TEXT NOT NULL", "initial_fen INTEGER NOT NULL"),
    std::make_pair("draw_offer_by TEXT,", ""),
    std::make_pair("draw_offer_by TEXT,", "draw_offer_by TEXT, unexpected TEXT,"),
    std::make_pair("increment_ms INTEGER NOT NULL DEFAULT 0", "increment_ms INTEGER NOT NULL DEFAULT 9"),
    std::make_pair("version INTEGER PRIMARY KEY CHECK (version > 0)", "version INTEGER PRIMARY KEY"),
    std::make_pair("version INTEGER NOT NULL DEFAULT 1 CHECK (version > 0)",
                   "version INTEGER NOT NULL DEFAULT 1"),
    std::make_pair("ply INTEGER NOT NULL CHECK (ply > 0)", "ply INTEGER NOT NULL"),
    std::make_pair("REFERENCES games(game_id) ON DELETE CASCADE", ""),
    std::make_pair("ON DELETE CASCADE", "ON DELETE RESTRICT"),
    std::make_pair("PRIMARY KEY (game_id, ply)", "PRIMARY KEY (ply, game_id)"),
    std::make_pair("PRIMARY KEY (principal_id, operation, idempotency_key)",
                   "PRIMARY KEY (principal_id, idempotency_key, operation)")));

  TEST_F(SqliteDatabaseTest, MissingTableIsNotSilentlyRecreated)
  {
    seed_valid();
    seed("DROP TABLE idempotency_records;");
    expect_rejected_unchanged();
  }

  TEST_F(SqliteDatabaseTest, MissingVersionTableDoesNotUpgradePartialSchema)
  {
    seed_valid();
    seed("DROP TABLE schema_version;");
    expect_rejected_unchanged();
  }

  TEST_F(SqliteDatabaseTest, ViewCannotStandInForATable)
  {
    seed_valid();
    seed("DROP TABLE idempotency_records; "
         "CREATE VIEW idempotency_records AS SELECT 'principal' AS principal_id;");
    expect_rejected_unchanged();
  }

  TEST_F(SqliteDatabaseTest, UnexpectedTriggerIsRejected)
  {
    seed_valid();
    seed("CREATE TRIGGER unexpected AFTER INSERT ON games BEGIN DELETE FROM games; END;");
    expect_rejected_unchanged();
  }

  TEST_F(SqliteDatabaseTest, VersionMustBeExactlyOneRowWithCurrentVersionTwo)
  {
    for (const char* version_sql : {"DELETE FROM schema_version;",
                                   "UPDATE schema_version SET version=1;",
                                   "UPDATE schema_version SET version=4294967297;",
                                   "INSERT INTO schema_version VALUES(3);"})
    {
      seed("DROP TABLE IF EXISTS game_moves; DROP TABLE IF EXISTS games; "
           "DROP TABLE IF EXISTS idempotency_records; DROP TABLE IF EXISTS schema_version;");
      seed_valid();
      seed(version_sql);
      expect_rejected_unchanged();
    }
  }

  TEST_F(SqliteDatabaseTest, ReadinessChecksVersionAndRecoversAfterDataRepair)
  {
    SqliteDatabase database(":memory:");
    ASSERT_TRUE(database.is_ready());
    ASSERT_EQ(sqlite3_exec(database.handle(), "UPDATE schema_version SET version=3", nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_FALSE(database.is_ready());
    ASSERT_EQ(sqlite3_exec(database.handle(), "UPDATE schema_version SET version=2", nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_TRUE(database.is_ready());
    ASSERT_EQ(sqlite3_exec(database.handle(), "DELETE FROM schema_version", nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_FALSE(database.is_ready());
    ASSERT_EQ(sqlite3_exec(database.handle(), "INSERT INTO schema_version VALUES(2)", nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_TRUE(database.is_ready());
    ASSERT_EQ(sqlite3_exec(database.handle(), "INSERT INTO schema_version VALUES(3)", nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_FALSE(database.is_ready());
  }

  TEST_F(SqliteDatabaseTest, VersionOneDatabaseMigratesToClockSchemaVersionTwo)
  {
    auto legacy = schema_;
    const std::string clock_columns = ",\n        clock_white_ms INTEGER,\n        clock_black_ms INTEGER,\n        clock_started_at_ms INTEGER";
    const auto columns_position = legacy.find(clock_columns);
    ASSERT_NE(columns_position, std::string::npos);
    legacy.erase(columns_position, clock_columns.size());
    seed(legacy + "INSERT INTO schema_version VALUES(1);");
    seed("INSERT INTO games(game_id,white_principal,initial_fen,initial_ms,created_at,updated_at) "
         "VALUES('legacy-game','white','fen',60000,'created','updated');");
    SqliteDatabase migrated(path_.string());
    ASSERT_TRUE(migrated.is_ready());
    EXPECT_EQ(scalar(migrated.handle(), "SELECT version FROM schema_version"), 2);
    EXPECT_EQ(scalar(migrated.handle(), "SELECT clock_white_ms FROM games WHERE game_id='legacy-game'"), 60000);
    EXPECT_EQ(scalar(migrated.handle(), "SELECT clock_black_ms FROM games WHERE game_id='legacy-game'"), 60000);
    EXPECT_EQ(scalar(migrated.handle(), "SELECT clock_started_at_ms IS NULL FROM games WHERE game_id='legacy-game'"), 1);
  }

  TEST_F(SqliteDatabaseTest, ReadinessRejectsLiveSchemaChanges)
  {
    SqliteDatabase database(":memory:");
    ASSERT_TRUE(database.is_ready());
    ASSERT_EQ(sqlite3_exec(database.handle(), "ALTER TABLE games ADD COLUMN unexpected TEXT", nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_FALSE(database.is_ready());
  }

  TEST_F(SqliteDatabaseTest, ReadinessDetectsSchemaChangesFromAnotherConnection)
  {
    SqliteDatabase database(path_.string());
    ASSERT_TRUE(database.is_ready());
    seed("DROP TABLE game_moves;");
    EXPECT_FALSE(database.is_ready());
  }

  TEST_F(SqliteDatabaseTest, ReadinessRequiresConstraintEnforcement)
  {
    SqliteDatabase database(":memory:");
    ASSERT_TRUE(database.is_ready());
    ASSERT_EQ(sqlite3_exec(database.handle(), "PRAGMA foreign_keys=OFF", nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_FALSE(database.is_ready());
    ASSERT_EQ(sqlite3_exec(database.handle(), "PRAGMA foreign_keys=ON", nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_TRUE(database.is_ready());
    ASSERT_EQ(sqlite3_exec(database.handle(), "PRAGMA ignore_check_constraints=ON", nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_FALSE(database.is_ready());
    ASSERT_EQ(sqlite3_exec(database.handle(), "PRAGMA ignore_check_constraints=OFF", nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_TRUE(database.is_ready());
  }

  TEST_F(SqliteDatabaseTest, ReadinessFailsWhenStorageReadsFail)
  {
    SqliteDatabase database(":memory:");
    ASSERT_TRUE(database.is_ready());
    ASSERT_EQ(sqlite3_set_authorizer(database.handle(),
      [](void*, int action, const char* table, const char*, const char*, const char*)
      {
        return action == SQLITE_READ && table != nullptr && std::string(table) == "games"
          ? SQLITE_DENY : SQLITE_OK;
      }, nullptr), SQLITE_OK);
    EXPECT_FALSE(database.is_ready());
    ASSERT_EQ(sqlite3_set_authorizer(database.handle(), nullptr, nullptr), SQLITE_OK);
    EXPECT_TRUE(database.is_ready());
  }

  TEST_F(SqliteDatabaseTest, ReadinessSynchronizesWithApplicationMutex)
  {
    SqliteDatabase database(":memory:");
    std::unique_lock<std::mutex> lock(database.mutex());
    std::promise<void> started;
    auto entered = started.get_future();
    auto readiness = std::async(std::launch::async, [&database, &started]
    {
      started.set_value();
      return database.is_ready();
    });
    entered.wait();
    EXPECT_EQ(readiness.wait_for(std::chrono::milliseconds(20)), std::future_status::timeout);
    lock.unlock();
    EXPECT_TRUE(readiness.get());
    std::lock_guard<std::mutex> application_lock(database.mutex());
    EXPECT_EQ(scalar(database.handle(), "SELECT version FROM schema_version"), 2);
  }

  TEST_F(SqliteDatabaseTest, OpenFailureIsNotReady)
  {
    SqliteDatabase database((path_ / "missing-parent" / "database.sqlite").string());
    EXPECT_FALSE(database.is_ready());
    EXPECT_EQ(database.handle(), nullptr);
  }
}