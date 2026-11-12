#include "session_revocation_store.h"
#include <sqlite3.h>
#include <array>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#include <vector>
#elif defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#else
#include <openssl/evp.h>
#endif

namespace Chess::Backend
{
  namespace
  {
    constexpr char schema[] = "CREATE TABLE revocations("
        "digest TEXT PRIMARY KEY NOT NULL CHECK(length(digest)=64 "
        "AND digest NOT GLOB '*[^0-9a-f]*'), expires INTEGER NOT NULL)";

    [[noreturn]] void unavailable()
    {
      throw std::runtime_error("Session revocation storage unavailable.");
    }

    void require(bool condition)
    {
      if (!condition) unavailable();
    }

    std::string digest_token(std::string_view token)
    {
      require(token.size() == 64);
      for (const char character : token)
        require((character >= '0' && character <= '9') || (character >= 'a' && character <= 'f')
            || (character >= 'A' && character <= 'F'));
      std::array<unsigned char, 32> bytes{};
#ifdef _WIN32
      struct AlgorithmCloser
      {
        void operator()(void* handle) const { BCryptCloseAlgorithmProvider(handle, 0); }
      };
      struct HashCloser
      {
        void operator()(void* handle) const { BCryptDestroyHash(handle); }
      };
      BCRYPT_ALG_HANDLE raw_algorithm = nullptr;
      const auto opened = BCryptOpenAlgorithmProvider(&raw_algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
      const std::unique_ptr<void, AlgorithmCloser> algorithm(raw_algorithm);
      require(opened == 0);
      ULONG object_size = 0;
      ULONG returned = 0;
      require(BCryptGetProperty(algorithm.get(), BCRYPT_OBJECT_LENGTH,
          reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &returned, 0) == 0);
      require(returned == sizeof(object_size) && object_size > 0);
      std::vector<unsigned char> object(object_size);
      BCRYPT_HASH_HANDLE raw_hash = nullptr;
      const auto created = BCryptCreateHash(algorithm.get(), &raw_hash, object.data(), object_size,
                                            nullptr, 0, 0);
      const std::unique_ptr<void, HashCloser> hash(raw_hash);
      require(created == 0);
      require(BCryptHashData(hash.get(), reinterpret_cast<PUCHAR>(const_cast<char*>(token.data())),
                            static_cast<ULONG>(token.size()), 0) == 0);
      require(BCryptFinishHash(hash.get(), bytes.data(), static_cast<ULONG>(bytes.size()), 0) == 0);
#elif defined(__APPLE__)
      require(CC_SHA256(token.data(), static_cast<CC_LONG>(token.size()), bytes.data()) != nullptr);
#else
      unsigned int length = 0;
      require(EVP_Digest(token.data(), token.size(), bytes.data(), &length, EVP_sha256(), nullptr) == 1);
      require(length == bytes.size());
#endif
      constexpr char hex[] = "0123456789abcdef";
      std::string result;
      result.reserve(64);
      for (const auto value : bytes)
      {
        result.push_back(hex[value >> 4]);
        result.push_back(hex[value & 15]);
      }
      return result;
    }

    struct DatabaseCloser
    {
      void operator()(sqlite3* database) const { sqlite3_close_v2(database); }
    };

    class Statement
    {
      public:
        Statement(sqlite3* database, const char* sql)
        {
          sqlite3_stmt* raw = nullptr;
          const auto result = sqlite3_prepare_v2(database, sql, -1, &raw, nullptr);
          statement_.reset(raw);
          require(result == SQLITE_OK);
        }

        void text(int position, std::string_view value)
        {
          require(value.size() <= 64);
          require(sqlite3_bind_text(statement_.get(), position, value.data(),
                                   static_cast<int>(value.size()), SQLITE_TRANSIENT) == SQLITE_OK);
        }

        void integer(int position, std::int64_t value)
        {
          require(sqlite3_bind_int64(statement_.get(), position, value) == SQLITE_OK);
        }

        bool row()
        {
          const auto result = sqlite3_step(statement_.get());
          require(result == SQLITE_ROW || result == SQLITE_DONE);
          return result == SQLITE_ROW;
        }

        void done() { require(!row()); }

        std::int64_t integer(int column) const
        {
          require(sqlite3_column_type(statement_.get(), column) == SQLITE_INTEGER);
          return sqlite3_column_int64(statement_.get(), column);
        }

        std::string text(int column) const
        {
          require(sqlite3_column_type(statement_.get(), column) == SQLITE_TEXT);
          const auto* value = sqlite3_column_text(statement_.get(), column);
          require(value != nullptr);
          return std::string(reinterpret_cast<const char*>(value),
                             static_cast<std::size_t>(sqlite3_column_bytes(statement_.get(), column)));
        }

      private:
        struct Closer
        {
          void operator()(sqlite3_stmt* statement) const { sqlite3_finalize(statement); }
        };
        std::unique_ptr<sqlite3_stmt, Closer> statement_;
    };

    void execute(sqlite3* database, const char* sql)
    {
      require(sqlite3_exec(database, sql, nullptr, nullptr, nullptr) == SQLITE_OK);
    }

    std::int64_t scalar_integer(sqlite3* database, const char* sql)
    {
      Statement statement(database, sql);
      require(statement.row());
      const auto result = statement.integer(0);
      statement.done();
      return result;
    }

    std::string scalar_text(sqlite3* database, const char* sql)
    {
      Statement statement(database, sql);
      require(statement.row());
      const auto result = statement.text(0);
      statement.done();
      return result;
    }

    class Transaction
    {
      public:
        explicit Transaction(sqlite3* database) : database_(database)
        {
          execute(database_, "BEGIN IMMEDIATE");
        }
        ~Transaction()
        {
          if (active_) sqlite3_exec(database_, "ROLLBACK", nullptr, nullptr, nullptr);
        }
        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;

        void commit()
        {
          execute(database_, "COMMIT");
          active_ = false;
        }

      private:
        sqlite3* database_;
        bool active_ = true;
    };

    void validate_schema(sqlite3* database)
    {
      require(scalar_integer(database, "PRAGMA user_version") == 1);
      require(scalar_integer(database,
          "SELECT count(*) FROM sqlite_schema WHERE name NOT LIKE 'sqlite_%'") == 1);
      require(scalar_text(database,
          "SELECT sql FROM sqlite_schema WHERE type='table' AND name='revocations'") == schema);
    }
  }

  struct SqliteSessionRevocationStore::State
  {
      std::unique_ptr<sqlite3, DatabaseCloser> database;
      std::filesystem::path path;
      std::size_t max_records;
      std::mutex mutex;

      void ready() const
      {
        std::error_code error;
        require(std::filesystem::is_regular_file(path, error) && !error);
        validate_schema(database.get());
        require(scalar_text(database.get(), "PRAGMA journal_mode") == "wal");
        require(scalar_integer(database.get(), "PRAGMA synchronous") == 2);
        require(scalar_integer(database.get(), "PRAGMA trusted_schema") == 0);
        require(scalar_integer(database.get(), "PRAGMA foreign_keys") == 1);
      }
  };

  SqliteSessionRevocationStore::SqliteSessionRevocationStore(const std::string& path,
                                                             std::size_t max_records)
      : state_(std::make_unique<State>())
  {
    require(!path.empty() && path.find('\0') == std::string::npos && path != ":memory:"
        && path.compare(0, 5, "file:") != 0 && max_records > 0 && max_records <= 65536);
    state_->path = std::filesystem::absolute(
        std::filesystem::path(std::u8string(path.begin(), path.end())));
    state_->max_records = max_records;
    const auto utf8 = state_->path.u8string();
    const std::string filename(utf8.begin(), utf8.end());
    sqlite3* raw = nullptr;
    const auto opened = sqlite3_open_v2(filename.c_str(), &raw,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
    state_->database.reset(raw);
    require(opened == SQLITE_OK);
    auto* database = state_->database.get();
    require(sqlite3_busy_timeout(database, 1000) == SQLITE_OK);
    int defensive = 0;
    require(sqlite3_db_config(database, SQLITE_DBCONFIG_DEFENSIVE, 1, &defensive) == SQLITE_OK
        && defensive == 1);
    execute(database, "PRAGMA trusted_schema=OFF");
    execute(database, "PRAGMA foreign_keys=ON");
    {
      Transaction transaction(database);
      if (scalar_integer(database, "SELECT count(*) FROM sqlite_schema") == 0
          && scalar_integer(database, "PRAGMA user_version") == 0)
      {
        execute(database, schema);
        execute(database, "PRAGMA user_version=1");
      }
      validate_schema(database);
      transaction.commit();
    }
    require(scalar_text(database, "PRAGMA journal_mode=WAL") == "wal");
    execute(database, "PRAGMA synchronous=FULL");
    state_->ready();
    require(scalar_text(database, "PRAGMA quick_check(1)") == "ok");
    require(scalar_integer(database, "SELECT count(*) FROM revocations")
        <= static_cast<std::int64_t>(max_records));
  }

  SqliteSessionRevocationStore::~SqliteSessionRevocationStore() = default;

  bool SqliteSessionRevocationStore::is_ready() const noexcept
  {
    try
    {
      std::lock_guard<std::mutex> lock(state_->mutex);
      state_->ready();
      return true;
    }
    catch (...)
    {
      return false;
    }
  }

  bool SqliteSessionRevocationStore::is_revoked(std::string_view token, std::int64_t now)
  {
    const auto digest = digest_token(token);
    std::lock_guard<std::mutex> lock(state_->mutex);
    auto* database = state_->database.get();
    Statement lookup(database, "SELECT expires FROM revocations WHERE digest=?1");
    lookup.text(1, digest);
    bool revoked = false;
    if (lookup.row())
    {
      revoked = lookup.integer(0) > now;
      lookup.done();
    }
    return revoked;
  }

  void SqliteSessionRevocationStore::revoke(std::string_view token, std::int64_t expires)
  {
    const auto digest = digest_token(token);
    require(expires > 0);
    std::lock_guard<std::mutex> lock(state_->mutex);
    auto* database = state_->database.get();
    Transaction transaction(database);
    state_->ready();
    Statement cleanup(database,
      "DELETE FROM revocations WHERE digest IN "
      "(SELECT digest FROM revocations WHERE expires<=?1 LIMIT 64)");
    cleanup.integer(1, expires);
    cleanup.done();
    const auto count = scalar_integer(database, "SELECT count(*) FROM revocations");
    require(count <= static_cast<std::int64_t>(state_->max_records));
    Statement existing(database, "SELECT expires FROM revocations WHERE digest=?1");
    existing.text(1, digest);
    const bool present = existing.row();
    if (present) existing.done();
    require(present || count < static_cast<std::int64_t>(state_->max_records));
    Statement insert(database,
        "INSERT INTO revocations(digest,expires) VALUES(?1,?2) "
        "ON CONFLICT(digest) DO UPDATE SET expires=max(expires,excluded.expires)");
    insert.text(1, digest);
    insert.integer(2, expires);
    insert.done();
    transaction.commit();
  }
}