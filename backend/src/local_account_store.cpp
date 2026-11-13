#include "local_account_store.h"
#include <sqlite3.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#elif defined(__APPLE__)
#include <CommonCrypto/CommonKeyDerivation.h>
#else
#include <openssl/evp.h>
#endif

namespace Chess::Backend
{
  namespace
  {
    constexpr int pbkdf2_iterations = 600000;
    constexpr int salt_bytes = 16;
    constexpr int hash_bytes = 32;
    constexpr int current_hash_version = 1;
    constexpr sqlite3_int64 maximum_accounts = 100000;
    constexpr char schema_v1[] = "CREATE TABLE accounts("
        "username TEXT PRIMARY KEY NOT NULL CHECK(length(username) BETWEEN 3 AND 32),"
        "salt BLOB NOT NULL CHECK(length(salt)=16),"
        "password_hash BLOB NOT NULL CHECK(length(password_hash)=32),"
        "created_at INTEGER NOT NULL) WITHOUT ROWID";
    constexpr char schema[] = "CREATE TABLE accounts("
      "username TEXT PRIMARY KEY NOT NULL CHECK(length(username) BETWEEN 3 AND 32),"
      "salt BLOB NOT NULL CHECK(length(salt)=16),"
      "password_hash BLOB NOT NULL CHECK(length(password_hash)=32),"
      "created_at INTEGER NOT NULL, "
      "hash_version INTEGER NOT NULL DEFAULT 1 CHECK(hash_version BETWEEN 1 AND 10)) WITHOUT ROWID";

    bool valid_username(std::string_view username)
    {
      return username.size() >= 3 && username.size() <= 32
          && std::all_of(username.begin(), username.end(), [](char value) {
            return (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9')
                || value == '.' || value == '_' || value == '-';
          });
    }

    std::optional<std::string> normalize_username(std::string_view username)
    {
      if (username.size() < 3 || username.size() > 32) return std::nullopt;
      std::string normalized;
      normalized.reserve(username.size());
      for (const unsigned char value : username)
      {
        if (value >= 'A' && value <= 'Z') normalized.push_back(static_cast<char>(value + ('a' - 'A')));
        else if ((value >= 'a' && value <= 'z') || (value >= '0' && value <= '9')
                 || value == '.' || value == '_' || value == '-') normalized.push_back(static_cast<char>(value));
        else return std::nullopt;
      }
      return valid_username(normalized) ? std::optional<std::string>(std::move(normalized)) : std::nullopt;
    }

    bool valid_password(std::string_view password)
    {
      return password.size() >= 12 && password.size() <= 1024
          && password.find('\0') == std::string_view::npos;
    }

    bool random_bytes(unsigned char* output, std::size_t length)
    {
#ifdef _WIN32
      return BCryptGenRandom(nullptr, output, static_cast<ULONG>(length), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
#else
      std::ifstream random("/dev/urandom", std::ios::binary);
      return random.read(reinterpret_cast<char*>(output), static_cast<std::streamsize>(length)).good();
#endif
    }

    bool derive_password(std::string_view password, const unsigned char* salt, unsigned char* output)
    {
#ifdef _WIN32
      BCRYPT_ALG_HANDLE algorithm = nullptr;
      if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0)
        return false;
      const auto result = BCryptDeriveKeyPBKDF2(algorithm,
          reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())), static_cast<ULONG>(password.size()),
          const_cast<PUCHAR>(salt), salt_bytes, pbkdf2_iterations, output, hash_bytes, 0);
      BCryptCloseAlgorithmProvider(algorithm, 0);
      return result == 0;
#elif defined(__APPLE__)
      return CCKeyDerivationPBKDF(kCCPBKDF2, password.data(), password.size(), salt, salt_bytes,
          kCCPRFHmacAlgSHA256, pbkdf2_iterations, output, hash_bytes) == kCCSuccess;
#else
      return PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()), salt, salt_bytes,
          pbkdf2_iterations, EVP_sha256(), hash_bytes, output) == 1;
#endif
    }

    bool constant_time_equal(const unsigned char* left, const unsigned char* right, std::size_t length)
    {
      unsigned char difference = 0;
      for (std::size_t index = 0; index < length; ++index) difference |= left[index] ^ right[index];
      return difference == 0;
    }

    std::string database_path(const std::filesystem::path& path)
    {
      const auto value = path.u8string();
      return std::string(value.begin(), value.end());
    }

    bool execute(sqlite3* database, const char* sql)
    {
      return sqlite3_exec(database, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
    }

    bool scalar_integer_is(sqlite3* database, const char* sql, sqlite3_int64 expected)
    {
      sqlite3_stmt* statement = nullptr;
      if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) return false;
      const bool valid = sqlite3_step(statement) == SQLITE_ROW
          && sqlite3_column_type(statement, 0) == SQLITE_INTEGER
          && sqlite3_column_int64(statement, 0) == expected
          && sqlite3_step(statement) == SQLITE_DONE;
      sqlite3_finalize(statement);
      return valid;
    }

    bool scalar_text_is(sqlite3* database, const char* sql, std::string_view expected)
    {
      sqlite3_stmt* statement = nullptr;
      if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) return false;
      const auto* value = sqlite3_step(statement) == SQLITE_ROW
          ? reinterpret_cast<const char*>(sqlite3_column_text(statement, 0)) : nullptr;
      const bool valid = value != nullptr && std::string_view(value) == expected
          && sqlite3_step(statement) == SQLITE_DONE;
      sqlite3_finalize(statement);
      return valid;
    }

    bool schema_objects_match(sqlite3* database, int version, const char* expected_schema)
    {
      return scalar_integer_is(database,
          "SELECT count(*) FROM sqlite_schema WHERE name NOT LIKE 'sqlite_%'", 1)
          && scalar_integer_is(database, "PRAGMA user_version", version)
          && scalar_text_is(database,
              "SELECT sql FROM sqlite_schema WHERE type='table' AND name='accounts'", expected_schema);
    }

    bool schema_v1_valid(sqlite3* database)
    {
      return schema_objects_match(database, 1, schema_v1);
    }

    bool schema_valid(sqlite3* database)
    {
      return schema_objects_match(database, 2, schema);
    }

    void secure_clear(unsigned char* bytes, std::size_t length)
    {
      volatile unsigned char* value = bytes;
      while (length-- > 0) *value++ = 0;
    }
  }

  struct SqliteLocalAccountStore::State
  {
      sqlite3* database = nullptr;
      std::filesystem::path path;
      mutable std::mutex mutex;

      ~State() { if (database != nullptr) sqlite3_close_v2(database); }

      bool ready() const noexcept
      {
        std::error_code error;
        int defensive = 0;
        return database != nullptr && std::filesystem::is_regular_file(path, error) && !error
            && schema_valid(database) && sqlite3_db_readonly(database, "main") == 0
            && scalar_text_is(database, "PRAGMA journal_mode", "wal")
            && scalar_integer_is(database, "PRAGMA synchronous", 2)
            && scalar_integer_is(database, "PRAGMA trusted_schema", 0)
            && scalar_integer_is(database, "PRAGMA foreign_keys", 1)
            && sqlite3_db_config(database, SQLITE_DBCONFIG_DEFENSIVE, -1, &defensive) == SQLITE_OK
            && defensive == 1;
      }
  };

  SqliteLocalAccountStore::SqliteLocalAccountStore(const std::string& path)
      : state_(std::make_unique<State>())
  {
    if (path.empty() || path.find('\0') != std::string::npos || path == ":memory:" || path.rfind("file:", 0) == 0)
      return;
    state_->path = std::filesystem::absolute(std::filesystem::path(std::u8string(path.begin(), path.end())));
    if (sqlite3_open_v2(database_path(state_->path).c_str(), &state_->database,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) return;
    sqlite3_extended_result_codes(state_->database, 1);
    if (sqlite3_busy_timeout(state_->database, 1000) != SQLITE_OK
        || !execute(state_->database, "PRAGMA trusted_schema=OFF")
        || !execute(state_->database, "PRAGMA journal_mode=WAL")
        || !execute(state_->database, "PRAGMA synchronous=FULL")) return;
    int defensive = 0;
    if (sqlite3_db_config(state_->database, SQLITE_DBCONFIG_DEFENSIVE, 1, &defensive) != SQLITE_OK || defensive != 1)
      return;
    if (!execute(state_->database, "BEGIN IMMEDIATE")) return;
    sqlite3_stmt* count_statement = nullptr;
    bool empty = false;
    if (sqlite3_prepare_v2(state_->database, "SELECT count(*) FROM sqlite_schema WHERE name NOT LIKE 'sqlite_%'",
                           -1, &count_statement, nullptr) == SQLITE_OK
        && sqlite3_step(count_statement) == SQLITE_ROW)
      empty = sqlite3_column_int64(count_statement, 0) == 0;
    sqlite3_finalize(count_statement);
    bool initialized = false;
    if (empty)
    {
      initialized = execute(state_->database, schema) && execute(state_->database, "PRAGMA user_version=2");
    }
    else if (schema_v1_valid(state_->database))
    {
      initialized = execute(state_->database,
          "ALTER TABLE accounts ADD COLUMN hash_version INTEGER NOT NULL DEFAULT 1 "
          "CHECK(hash_version BETWEEN 1 AND 10)")
          && execute(state_->database, "PRAGMA user_version=2");
    }
    else
    {
      initialized = schema_valid(state_->database);
    }
    if (initialized) initialized = execute(state_->database, "COMMIT");
    if (!initialized) execute(state_->database, "ROLLBACK");
  }

  SqliteLocalAccountStore::~SqliteLocalAccountStore() = default;

  bool SqliteLocalAccountStore::is_ready() const noexcept
  {
    try
    {
      std::lock_guard<std::mutex> lock(state_->mutex);
      return state_->ready();
    }
    catch (...) { return false; }
  }

  AccountCreateResult SqliteLocalAccountStore::create_account(std::string_view username,
                                                              std::string_view password) noexcept
  {
    try
    {
      const auto normalized = normalize_username(username);
      if (!normalized || !valid_password(password) || !is_ready()) return AccountCreateResult::Unavailable;
      std::array<unsigned char, salt_bytes> salt{};
      std::array<unsigned char, hash_bytes> hash{};
      if (!random_bytes(salt.data(), salt.size()) || !derive_password(password, salt.data(), hash.data()))
        return AccountCreateResult::Unavailable;
      std::lock_guard<std::mutex> lock(state_->mutex);
      if (!execute(state_->database, "BEGIN IMMEDIATE"))
      {
        secure_clear(hash.data(), hash.size());
        return AccountCreateResult::Unavailable;
      }
      sqlite3_stmt* count_statement = nullptr;
      const bool count_ready = sqlite3_prepare_v2(state_->database,
          "SELECT count(*) FROM accounts", -1, &count_statement, nullptr) == SQLITE_OK
          && sqlite3_step(count_statement) == SQLITE_ROW;
      const auto account_count = count_ready ? sqlite3_column_int64(count_statement, 0) : maximum_accounts;
      sqlite3_finalize(count_statement);
      if (!count_ready || account_count >= maximum_accounts)
      {
        execute(state_->database, "ROLLBACK");
        secure_clear(hash.data(), hash.size());
        return AccountCreateResult::Unavailable;
      }
      sqlite3_stmt* statement = nullptr;
      const bool prepared = sqlite3_prepare_v2(state_->database,
          "INSERT INTO accounts(username,salt,password_hash,created_at,hash_version) "
          "VALUES(?1,?2,?3,unixepoch(),?4)",
          -1, &statement, nullptr) == SQLITE_OK;
      const bool bound = prepared
          && sqlite3_bind_text(statement, 1, normalized->c_str(), static_cast<int>(normalized->size()), SQLITE_TRANSIENT) == SQLITE_OK
          && sqlite3_bind_blob(statement, 2, salt.data(), static_cast<int>(salt.size()), SQLITE_TRANSIENT) == SQLITE_OK
          && sqlite3_bind_blob(statement, 3, hash.data(), static_cast<int>(hash.size()), SQLITE_TRANSIENT) == SQLITE_OK
          && sqlite3_bind_int(statement, 4, current_hash_version) == SQLITE_OK;
      const int result = bound ? sqlite3_step(statement) : SQLITE_ERROR;
      sqlite3_finalize(statement);
      const int error = sqlite3_extended_errcode(state_->database);
      secure_clear(hash.data(), hash.size());
      if (result == SQLITE_DONE && execute(state_->database, "COMMIT")) return AccountCreateResult::Created;
      execute(state_->database, "ROLLBACK");
      if (error == SQLITE_CONSTRAINT_PRIMARYKEY || error == SQLITE_CONSTRAINT_UNIQUE)
        return AccountCreateResult::AlreadyExists;
      return AccountCreateResult::Unavailable;
    }
    catch (...) { return AccountCreateResult::Unavailable; }
  }

  std::optional<Principal> SqliteLocalAccountStore::authenticate(std::string_view username,
                                                                 std::string_view password) noexcept
  {
    try
    {
      const auto normalized = normalize_username(username);
      if (!normalized || !valid_password(password) || !is_ready()) return std::nullopt;
      std::array<unsigned char, salt_bytes> salt{};
      std::array<unsigned char, hash_bytes> expected{};
      bool found = false;
      {
        std::lock_guard<std::mutex> lock(state_->mutex);
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(state_->database,
          "SELECT salt,password_hash,hash_version FROM accounts WHERE username=?1", -1, &statement, nullptr) != SQLITE_OK)
          return std::nullopt;
        if (sqlite3_bind_text(statement, 1, normalized->c_str(), static_cast<int>(normalized->size()), SQLITE_TRANSIENT) == SQLITE_OK
            && sqlite3_step(statement) == SQLITE_ROW
            && sqlite3_column_type(statement, 0) == SQLITE_BLOB && sqlite3_column_bytes(statement, 0) == salt_bytes
            && sqlite3_column_type(statement, 1) == SQLITE_BLOB && sqlite3_column_bytes(statement, 1) == hash_bytes)
        {
          const auto stored_version = sqlite3_column_int(statement, 2);
          if (sqlite3_column_type(statement, 2) == SQLITE_INTEGER && stored_version == current_hash_version)
          {
            std::copy_n(static_cast<const unsigned char*>(sqlite3_column_blob(statement, 0)), salt.size(), salt.begin());
            std::copy_n(static_cast<const unsigned char*>(sqlite3_column_blob(statement, 1)), expected.size(), expected.begin());
            found = true;
          }
        }
        sqlite3_finalize(statement);
      }
      static constexpr std::array<unsigned char, salt_bytes> dummy_salt{
        0x91, 0x33, 0x28, 0xA4, 0x5D, 0x6C, 0x0B, 0xE2, 0x47, 0xD9, 0xAA, 0x11, 0x62, 0xF0, 0xC5, 0x7B};
      if (!found) salt = dummy_salt;
      std::array<unsigned char, hash_bytes> actual{};
      const bool derived = derive_password(password, salt.data(), actual.data());
      const bool matches = found && derived && constant_time_equal(actual.data(), expected.data(), actual.size());
      secure_clear(actual.data(), actual.size());
      secure_clear(expected.data(), expected.size());
      return matches ? std::optional<Principal>(Principal{*normalized}) : std::nullopt;
    }
    catch (...) { return std::nullopt; }
  }
}