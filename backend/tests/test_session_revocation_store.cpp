#include "session_revocation_store.h"
#include "session_authenticator.h"
#include <gtest/gtest.h>
#include <sqlite3.h>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>

namespace
{
  using namespace Chess::Backend;

  class TemporaryDatabasePath
  {
    public:
      TemporaryDatabasePath()
      {
        static std::atomic<unsigned> next{0};
        path_ = std::filesystem::temp_directory_path()
            / ("ichess-revocations-" + std::to_string(next.fetch_add(1)) + ".sqlite");
      }

      ~TemporaryDatabasePath()
      {
        std::error_code error;
        std::filesystem::remove(path_, error);
        std::filesystem::remove(path_.string() + "-wal", error);
        std::filesystem::remove(path_.string() + "-shm", error);
      }

      const std::filesystem::path& get() const { return path_; }

    private:
      std::filesystem::path path_;
  };

  TEST(SessionRevocationStoreTest, PersistsRevocationsAcrossStoreRestart)
  {
    TemporaryDatabasePath path;
    const std::string token(64, 'a');
    {
      SqliteSessionRevocationStore store(path.get().string());
      EXPECT_FALSE(store.is_revoked(token, 100));
      store.revoke(token, 200);
      EXPECT_TRUE(store.is_revoked(token, 100));
    }
    {
      SqliteSessionRevocationStore store(path.get().string());
      EXPECT_TRUE(store.is_revoked(token, 100));
      EXPECT_FALSE(store.is_revoked(token, 200));
      EXPECT_FALSE(store.is_revoked(std::string(64, 'b'), 100));
    }
  }

  TEST(SessionRevocationStoreTest, ImportedSessionRevocationSurvivesAuthenticatorRestart)
  {
    TemporaryDatabasePath path;
    auto store = std::make_shared<SqliteSessionRevocationStore>(path.get().string());
    const auto now = std::chrono::system_clock::time_point(std::chrono::seconds(1700000000));
    const auto steady = std::chrono::steady_clock::time_point(std::chrono::seconds(100));
    const std::string token(64, 'c');
    {
      SessionAuthenticator auth({"https://chess.test"}, [now] { return now; },
                                [steady] { return steady; }, store);
      ASSERT_TRUE(auth.import_session(Principal{"alice"}, token, 1700000100));
      ASSERT_TRUE(auth.revoke_session(token));
    }
    {
      SessionAuthenticator auth({"https://chess.test"}, [now] { return now; },
                                [steady] { return steady; }, store);
      EXPECT_FALSE(auth.import_session(Principal{"alice"}, token, 1700000100));
    }
  }

  TEST(SessionRevocationStoreTest, ExpiredRecordsFreeCapacityForNewRevocations)
  {
    TemporaryDatabasePath path;
    SqliteSessionRevocationStore store(path.get().string(), 1);
    store.revoke(std::string(64, 'a'), 200);
    EXPECT_FALSE(store.is_revoked(std::string(64, 'a'), 200));
    store.revoke(std::string(64, 'b'), 300);
    EXPECT_TRUE(store.is_revoked(std::string(64, 'b'), 200));
  }

  TEST(SessionRevocationStoreTest, RejectsMalformedTokensWithoutExposingStorageDetails)
  {
    TemporaryDatabasePath path;
    SqliteSessionRevocationStore store(path.get().string());
    try
    {
      store.revoke("not-a-token", 200);
      FAIL();
    }
    catch (const std::exception& error)
    {
      EXPECT_STREQ(error.what(), "Session revocation storage unavailable.");
    }
  }

  TEST(SessionRevocationStoreTest, ReadinessDetectsSchemaLoss)
  {
    TemporaryDatabasePath path;
    SqliteSessionRevocationStore store(path.get().string());
    ASSERT_TRUE(store.is_ready());
    sqlite3* database = nullptr;
    ASSERT_EQ(sqlite3_open(path.get().string().c_str(), &database), SQLITE_OK);
    const auto result = sqlite3_exec(database, "DROP TABLE revocations", nullptr, nullptr, nullptr);
    sqlite3_close(database);
    ASSERT_EQ(result, SQLITE_OK);
    EXPECT_FALSE(store.is_ready());
  }

  TEST(SessionRevocationStoreTest, OnlineBackupPreservesRevocationState)
  {
    TemporaryDatabasePath source_path;
    TemporaryDatabasePath backup_path;
    const std::string token(64, 'd');
    {
      SqliteSessionRevocationStore source(source_path.get().string());
      ASSERT_TRUE(source.is_ready());
      source.revoke(token, 500);
      ASSERT_TRUE(source.backup_to(backup_path.get()));
      EXPECT_FALSE(source.backup_to(backup_path.get()));
    }
    SqliteSessionRevocationStore restored(backup_path.get().string());
    ASSERT_TRUE(restored.is_ready());
    EXPECT_TRUE(restored.is_revoked(token, 400));
    EXPECT_FALSE(restored.is_revoked(token, 500));
  }
}