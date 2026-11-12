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
}