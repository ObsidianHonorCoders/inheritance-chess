#include "local_account_store.h"
#include <gtest/gtest.h>
#include <sqlite3.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>

namespace
{
  using namespace Chess::Backend;

  class TemporaryAccountDatabase
  {
    public:
      TemporaryAccountDatabase()
      {
        static std::atomic<unsigned> sequence{0};
        path_ = std::filesystem::temp_directory_path()
          / ("ichess-accounts-"
            + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
            + "-" + std::to_string(sequence.fetch_add(1)) + ".sqlite");
      }

      ~TemporaryAccountDatabase()
      {
        std::error_code error;
        std::filesystem::remove(path_, error);
        std::filesystem::remove(path_.string() + "-wal", error);
        std::filesystem::remove(path_.string() + "-shm", error);
      }

      const std::filesystem::path& path() const { return path_; }

    private:
      std::filesystem::path path_;
  };

  TEST(LocalAccountStoreTest, CreatesCaseInsensitiveAccountsAndVerifiesPasswords)
  {
    TemporaryAccountDatabase path;
    SqliteLocalAccountStore accounts(path.path().string());
    ASSERT_TRUE(accounts.is_ready());
    EXPECT_EQ(accounts.create_account("Alice.Example", "correct horse battery staple"), AccountCreateResult::Created);
    EXPECT_EQ(accounts.create_account("alice.example", "another secure password"), AccountCreateResult::AlreadyExists);
    const auto principal = accounts.authenticate("ALICE.EXAMPLE", "correct horse battery staple");
    ASSERT_TRUE(principal);
    EXPECT_EQ(principal->id, "alice.example");
    EXPECT_FALSE(accounts.authenticate("alice.example", "a wrong password value"));
    EXPECT_FALSE(accounts.authenticate("unknown", "correct horse battery staple"));
  }

  TEST(LocalAccountStoreTest, ChangesPasswordOnlyAfterCurrentPasswordVerification)
  {
    TemporaryAccountDatabase path;
    SqliteLocalAccountStore accounts(path.path().string());
    ASSERT_TRUE(accounts.is_ready());
    ASSERT_EQ(accounts.create_account("change-user", "original secure password"), AccountCreateResult::Created);
    EXPECT_EQ(accounts.change_password("change-user", "incorrect secure password", "replacement secure password"),
              PasswordChangeResult::InvalidCurrentPassword);
    EXPECT_TRUE(accounts.authenticate("change-user", "original secure password"));
    EXPECT_EQ(accounts.change_password("change-user", "original secure password", "replacement secure password"),
              PasswordChangeResult::Changed);
    EXPECT_FALSE(accounts.authenticate("change-user", "original secure password"));
    EXPECT_TRUE(accounts.authenticate("change-user", "replacement secure password"));
  }

  TEST(LocalAccountStoreTest, AccountDeletionRequiresPasswordAndRemovesCredential)
  {
    TemporaryAccountDatabase path;
    SqliteLocalAccountStore accounts(path.path().string());
    ASSERT_TRUE(accounts.is_ready());
    ASSERT_EQ(accounts.create_account("delete-user", "account deletion password"), AccountCreateResult::Created);
    EXPECT_EQ(accounts.delete_account("delete-user", "wrong deletion password"), AccountDeleteResult::InvalidPassword);
    EXPECT_TRUE(accounts.authenticate("delete-user", "account deletion password"));
    EXPECT_EQ(accounts.delete_account("delete-user", "account deletion password"), AccountDeleteResult::Deleted);
    EXPECT_FALSE(accounts.authenticate("delete-user", "account deletion password"));
  }

  TEST(LocalAccountStoreTest, RejectsWeakPasswordsAndMalformedAccountNames)
  {
    TemporaryAccountDatabase path;
    SqliteLocalAccountStore accounts(path.path().string());
    ASSERT_TRUE(accounts.is_ready());
    for (const auto& [username, password] : {
        std::pair<std::string, std::string>{"ab", "correct horse battery staple"},
        {"bad name", "correct horse battery staple"},
        {"alice", "short"},
        {"alice", std::string(1025, 'x')}})
      EXPECT_EQ(accounts.create_account(username, password), AccountCreateResult::Unavailable);
  }

  TEST(LocalAccountStoreTest, PersistsAccountsAcrossRestart)
  {
    TemporaryAccountDatabase path;
    {
      SqliteLocalAccountStore accounts(path.path().string());
      ASSERT_TRUE(accounts.is_ready());
      EXPECT_EQ(accounts.create_account("persisted-user", "a long enough test password"), AccountCreateResult::Created);
    }
    {
      SqliteLocalAccountStore accounts(path.path().string());
      ASSERT_TRUE(accounts.is_ready());
      EXPECT_EQ(accounts.authenticate("persisted-user", "a long enough test password")->id, "persisted-user");
    }
  }

  TEST(LocalAccountStoreTest, UnexpectedSchemaObjectsFailReadiness)
  {
    TemporaryAccountDatabase path;
    SqliteLocalAccountStore accounts(path.path().string());
    ASSERT_TRUE(accounts.is_ready());
    sqlite3* database = nullptr;
    ASSERT_EQ(sqlite3_open(path.path().string().c_str(), &database), SQLITE_OK);
    const auto result = sqlite3_exec(database, "CREATE TABLE unexpected(value TEXT)", nullptr, nullptr, nullptr);
    sqlite3_close(database);
    ASSERT_EQ(result, SQLITE_OK);
    EXPECT_FALSE(accounts.is_ready());
  }

  TEST(LocalAccountStoreTest, MigratesLegacyPasswordRowsToExplicitHashVersion)
  {
    TemporaryAccountDatabase path;
    sqlite3* database = nullptr;
    ASSERT_EQ(sqlite3_open(path.path().string().c_str(), &database), SQLITE_OK);
    const auto created = sqlite3_exec(database,
        "CREATE TABLE accounts(username TEXT PRIMARY KEY NOT NULL CHECK(length(username) BETWEEN 3 AND 32),"
        "salt BLOB NOT NULL CHECK(length(salt)=16),password_hash BLOB NOT NULL CHECK(length(password_hash)=32),"
        "created_at INTEGER NOT NULL) WITHOUT ROWID;"
        "PRAGMA user_version=1;"
        "INSERT INTO accounts(username,salt,password_hash,created_at) "
        "VALUES('legacy-user',zeroblob(16),zeroblob(32),unixepoch());",
        nullptr, nullptr, nullptr);
    sqlite3_close(database);
    ASSERT_EQ(created, SQLITE_OK);

    SqliteLocalAccountStore accounts(path.path().string());
    ASSERT_TRUE(accounts.is_ready());
    ASSERT_EQ(sqlite3_open(path.path().string().c_str(), &database), SQLITE_OK);
    sqlite3_stmt* statement = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(database, "PRAGMA user_version", -1, &statement, nullptr), SQLITE_OK);
    ASSERT_EQ(sqlite3_step(statement), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(statement, 0), 2);
    sqlite3_finalize(statement);
    ASSERT_EQ(sqlite3_prepare_v2(database,
        "SELECT hash_version FROM accounts WHERE username='legacy-user'", -1, &statement, nullptr), SQLITE_OK);
    ASSERT_EQ(sqlite3_step(statement), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(statement, 0), 1);
    EXPECT_EQ(sqlite3_step(statement), SQLITE_DONE);
    sqlite3_finalize(statement);
    sqlite3_close(database);
  }

  TEST(LocalAccountStoreTest, OnlineBackupRestoresAccountsAndNeverOverwritesDestination)
  {
    TemporaryAccountDatabase source_path;
    TemporaryAccountDatabase backup_path;
    SqliteLocalAccountStore source(source_path.path().string());
    ASSERT_TRUE(source.is_ready());
    ASSERT_EQ(source.create_account("backup-user", "a password long enough"), AccountCreateResult::Created);
    EXPECT_TRUE(source.backup_to(backup_path.path()));
    EXPECT_FALSE(source.backup_to(backup_path.path()));
    SqliteLocalAccountStore restored(backup_path.path().string());
    ASSERT_TRUE(restored.is_ready());
    const auto principal = restored.authenticate("backup-user", "a password long enough");
    ASSERT_TRUE(principal);
    EXPECT_EQ(principal->id, "backup-user");
  }
}