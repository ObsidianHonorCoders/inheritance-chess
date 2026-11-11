#include "sqlite_database.h"

namespace Chess::Backend
{
  namespace
  {
    bool execute(sqlite3* database, const char* sql)
    {
      return sqlite3_exec(database, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
    }
  } // namespace

  SqliteDatabase::SqliteDatabase(const std::string& path)
  {
    if (sqlite3_open_v2(path.c_str(), &database_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                        nullptr) != SQLITE_OK)
    {
      if (database_ != nullptr)
      {
        sqlite3_close(database_);
        database_ = nullptr;
      }
      return;
    }

    sqlite3_extended_result_codes(database_, 1);
    sqlite3_busy_timeout(database_, 5000);

    if (!execute(database_, "PRAGMA foreign_keys = ON")
        || !execute(database_, "PRAGMA journal_mode = WAL")
        || !execute(database_, "PRAGMA synchronous = FULL")
        || !execute(database_, "PRAGMA trusted_schema = OFF"))
    {
      return;
    }

    if (sqlite3_db_config(database_, SQLITE_DBCONFIG_DEFENSIVE, 1, nullptr) != SQLITE_OK)
    {
      return;
    }

    const char* schema = R"sql(
      BEGIN IMMEDIATE;
      CREATE TABLE IF NOT EXISTS schema_version (
        version INTEGER PRIMARY KEY CHECK (version > 0)
      );
      INSERT INTO schema_version(version)
        SELECT 1 WHERE NOT EXISTS (SELECT 1 FROM schema_version);
      CREATE TABLE IF NOT EXISTS games (
        game_id TEXT PRIMARY KEY NOT NULL,
        white_principal TEXT NOT NULL,
        black_principal TEXT,
        initial_fen TEXT NOT NULL,
        initial_ms INTEGER,
        increment_ms INTEGER NOT NULL DEFAULT 0,
        version INTEGER NOT NULL DEFAULT 1 CHECK (version > 0),
        status TEXT NOT NULL DEFAULT 'ongoing',
        draw_offer_by TEXT,
        created_at TEXT NOT NULL,
        updated_at TEXT NOT NULL
      );
      CREATE TABLE IF NOT EXISTS game_moves (
        game_id TEXT NOT NULL REFERENCES games(game_id) ON DELETE CASCADE,
        ply INTEGER NOT NULL CHECK (ply > 0),
        uci TEXT NOT NULL,
        san TEXT NOT NULL,
        PRIMARY KEY (game_id, ply)
      );
      CREATE TABLE IF NOT EXISTS idempotency_records (
        principal_id TEXT NOT NULL,
        operation TEXT NOT NULL,
        idempotency_key TEXT NOT NULL,
        request_fingerprint TEXT NOT NULL,
        response_json TEXT NOT NULL,
        created_at TEXT NOT NULL,
        PRIMARY KEY (principal_id, operation, idempotency_key)
      );
      CREATE INDEX IF NOT EXISTS games_white_principal_idx ON games(white_principal);
      CREATE INDEX IF NOT EXISTS games_black_principal_idx ON games(black_principal);
      CREATE INDEX IF NOT EXISTS idempotency_created_at_idx ON idempotency_records(created_at);
    )sql";
    if (!execute(database_, schema))
    {
      execute(database_, "ROLLBACK");
      return;
    }

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_, "SELECT version FROM schema_version", -1, &statement, nullptr) == SQLITE_OK
      && sqlite3_step(statement) == SQLITE_ROW && sqlite3_column_int(statement, 0) == 1
      && sqlite3_step(statement) == SQLITE_DONE)
    {
      ready_ = true;
    }
    sqlite3_finalize(statement);
    if (!ready_ || !execute(database_, "COMMIT"))
    {
      execute(database_, "ROLLBACK");
      ready_ = false;
    }
  }

  SqliteDatabase::~SqliteDatabase()
  {
    if (database_ != nullptr)
    {
      sqlite3_close(database_);
    }
  }

  bool SqliteDatabase::is_ready() const
  {
    return ready_;
  }

  sqlite3* SqliteDatabase::handle() const
  {
    return database_;
  }

  std::mutex& SqliteDatabase::mutex() const
  {
    return mutex_;
  }
} // namespace Chess::Backend