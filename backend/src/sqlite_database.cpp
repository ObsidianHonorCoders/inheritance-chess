#include "sqlite_database.h"
#include "sqlite_backup.h"

#include <initializer_list>
#include <memory>

namespace Chess::Backend
{
  namespace
  {
    bool execute(sqlite3* database, const char* sql)
    {
      return sqlite3_exec(database, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
    }

    using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

    Statement prepare(sqlite3* database, const char* sql)
    {
      sqlite3_stmt* statement = nullptr;
      if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
      {
        sqlite3_finalize(statement);
        statement = nullptr;
      }
      return Statement(statement, sqlite3_finalize);
    }

    bool integer_value(sqlite3* database, const char* sql, int& value)
    {
      auto statement = prepare(database, sql);
      if (!statement || sqlite3_step(statement.get()) != SQLITE_ROW
          || sqlite3_column_type(statement.get(), 0) != SQLITE_INTEGER)
      {
        return false;
      }
      value = sqlite3_column_int(statement.get(), 0);
      return sqlite3_step(statement.get()) == SQLITE_DONE;
    }

    struct Column
    {
      const char* name;
      const char* type;
      int not_null;
      int primary_key;
      const char* default_value = nullptr;
    };

    bool text_equals(sqlite3_stmt* statement, int column, const char* expected)
    {
      const auto* actual = reinterpret_cast<const char*>(sqlite3_column_text(statement, column));
      return expected == nullptr ? actual == nullptr
                                 : actual != nullptr && sqlite3_stricmp(actual, expected) == 0;
    }

    bool columns_match(sqlite3* database, const char* sql, std::initializer_list<Column> columns)
    {
      auto statement = prepare(database, sql);
      if (!statement)
      {
        return false;
      }
      for (const auto& column : columns)
      {
        if (sqlite3_step(statement.get()) != SQLITE_ROW
            || !text_equals(statement.get(), 1, column.name)
            || !text_equals(statement.get(), 2, column.type)
            || sqlite3_column_int(statement.get(), 3) != column.not_null
            || !text_equals(statement.get(), 4, column.default_value)
            || sqlite3_column_int(statement.get(), 5) != column.primary_key
            || sqlite3_column_int(statement.get(), 6) != 0)
        {
          return false;
        }
      }
      return sqlite3_step(statement.get()) == SQLITE_DONE;
    }

    bool validate_schema(sqlite3* database)
    {
      int table_count = 0;
      int trigger_count = 0;
      if (!integer_value(database, "SELECT count(*) FROM sqlite_schema WHERE type = 'table' "
                         "AND name IN ('schema_version','games','game_moves','idempotency_records')", table_count)
          || table_count != 4
          || !integer_value(database, "SELECT count(*) FROM sqlite_schema WHERE type = 'trigger' "
                            "AND tbl_name IN ('schema_version','games','game_moves','idempotency_records')", trigger_count)
          || trigger_count != 0
          || !columns_match(database, "PRAGMA main.table_xinfo('schema_version')", {
               {"version", "INTEGER", 0, 1}})
          || !columns_match(database, "PRAGMA main.table_xinfo('games')", {
               {"game_id", "TEXT", 1, 1}, {"white_principal", "TEXT", 1, 0},
               {"black_principal", "TEXT", 0, 0}, {"initial_fen", "TEXT", 1, 0},
               {"initial_ms", "INTEGER", 0, 0}, {"increment_ms", "INTEGER", 1, 0, "0"},
               {"version", "INTEGER", 1, 0, "1"}, {"status", "TEXT", 1, 0, "'ongoing'"},
               {"draw_offer_by", "TEXT", 0, 0}, {"created_at", "TEXT", 1, 0},
               {"updated_at", "TEXT", 1, 0}})
          || !columns_match(database, "PRAGMA main.table_xinfo('game_moves')", {
               {"game_id", "TEXT", 1, 1}, {"ply", "INTEGER", 1, 2},
               {"uci", "TEXT", 1, 0}, {"san", "TEXT", 1, 0}})
          || !columns_match(database, "PRAGMA main.table_xinfo('idempotency_records')", {
               {"principal_id", "TEXT", 1, 1}, {"operation", "TEXT", 1, 2},
               {"idempotency_key", "TEXT", 1, 3}, {"request_fingerprint", "TEXT", 1, 0},
               {"response_json", "TEXT", 1, 0}, {"created_at", "TEXT", 1, 0}}))
      {
        return false;
      }

      auto foreign_key = prepare(database, "PRAGMA main.foreign_key_list('game_moves')");
      if (!foreign_key || sqlite3_step(foreign_key.get()) != SQLITE_ROW
          || sqlite3_column_int(foreign_key.get(), 1) != 0
          || !text_equals(foreign_key.get(), 2, "games")
          || !text_equals(foreign_key.get(), 3, "game_id")
          || !text_equals(foreign_key.get(), 4, "game_id")
          || !text_equals(foreign_key.get(), 5, "NO ACTION")
          || !text_equals(foreign_key.get(), 6, "CASCADE")
          || sqlite3_step(foreign_key.get()) != SQLITE_DONE)
      {
        return false;
      }

      if (!execute(database, "SAVEPOINT schema_probe"))
      {
        return false;
      }
      const bool valid_game = execute(database,
        "INSERT INTO games(game_id,white_principal,initial_fen,created_at,updated_at) "
        "VALUES (lower(hex(randomblob(32))),'schema-probe','schema-probe','','')");
      const auto rejects_check = [database](const char* sql)
      {
        return !execute(database, sql) && sqlite3_extended_errcode(database) == SQLITE_CONSTRAINT_CHECK;
      };
      const bool valid = valid_game
        && rejects_check("INSERT INTO schema_version(version) VALUES(0)")
        && rejects_check("INSERT INTO schema_version(version) VALUES(-1)")
        && rejects_check("UPDATE games SET version=0 WHERE rowid=last_insert_rowid()")
        && rejects_check("UPDATE games SET version=-1 WHERE rowid=last_insert_rowid()")
        && execute(database, "INSERT INTO game_moves(game_id,ply,uci,san) "
                            "SELECT game_id,1,'e2e4','e4' FROM games WHERE rowid=last_insert_rowid()")
        && rejects_check("UPDATE game_moves SET ply=0 WHERE rowid=last_insert_rowid()")
        && rejects_check("UPDATE game_moves SET ply=-1 WHERE rowid=last_insert_rowid()");
      const bool rolled_back = execute(database, "ROLLBACK TO schema_probe");
      const bool released = execute(database, "RELEASE schema_probe");
      return valid && rolled_back && released;
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
    if (sqlite3_busy_timeout(database_, 1000) != SQLITE_OK)
    {
      return;
    }

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
    if (!execute(database_, "BEGIN IMMEDIATE"))
    {
      return;
    }
    int object_count = 0;
    if (!integer_value(database_, "SELECT count(*) FROM sqlite_schema WHERE name NOT LIKE 'sqlite_%'", object_count)
        || (object_count == 0 && !execute(database_, schema))
        || !validate_schema(database_))
    {
      execute(database_, "ROLLBACK");
      return;
    }

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_, "SELECT version FROM schema_version", -1, &statement, nullptr) == SQLITE_OK
      && sqlite3_step(statement) == SQLITE_ROW && sqlite3_column_type(statement, 0) == SQLITE_INTEGER
      && sqlite3_column_int64(statement, 0) == 1
      && sqlite3_step(statement) == SQLITE_DONE)
    {
      initialized_ = true;
    }
    sqlite3_finalize(statement);
    if (!initialized_ || !integer_value(database_, "PRAGMA main.schema_version", schema_cookie_)
      || !execute(database_, "COMMIT"))
    {
      execute(database_, "ROLLBACK");
      initialized_ = false;
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
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_ || database_ == nullptr)
    {
      return false;
    }
    auto statement = prepare(database_,
      "SELECT version, (SELECT schema_version FROM pragma_schema_version), "
      "(SELECT foreign_keys FROM pragma_foreign_keys), "
      "(SELECT ignore_check_constraints FROM pragma_ignore_check_constraints), "
      "(SELECT game_id FROM main.games LIMIT 1), "
      "(SELECT ply FROM main.game_moves LIMIT 1), "
      "(SELECT principal_id FROM main.idempotency_records LIMIT 1) FROM main.schema_version");
    return statement && sqlite3_step(statement.get()) == SQLITE_ROW
      && sqlite3_column_type(statement.get(), 0) == SQLITE_INTEGER
      && sqlite3_column_int64(statement.get(), 0) == 1
      && sqlite3_column_type(statement.get(), 1) == SQLITE_INTEGER
      && sqlite3_column_int(statement.get(), 1) == schema_cookie_
      && sqlite3_column_int(statement.get(), 2) == 1
      && sqlite3_column_int(statement.get(), 3) == 0
      && sqlite3_step(statement.get()) == SQLITE_DONE;
  }

  bool SqliteDatabase::backup_to(const std::filesystem::path& destination) const noexcept
  {
    return sqlite_backup_to_file(database_, mutex_, destination);
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