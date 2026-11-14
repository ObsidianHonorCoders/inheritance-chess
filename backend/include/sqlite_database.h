#ifndef ICHESS_BACKEND_SQLITE_DATABASE
#define ICHESS_BACKEND_SQLITE_DATABASE

#include <sqlite3.h>
#include <filesystem>
#include <mutex>
#include <string>

namespace Chess::Backend
{
  class SqliteDatabase
  {
    public:
      explicit SqliteDatabase(const std::string& path);
      ~SqliteDatabase();

      SqliteDatabase(const SqliteDatabase&) = delete;
      SqliteDatabase& operator=(const SqliteDatabase&) = delete;

      bool is_ready() const;
      bool backup_to(const std::filesystem::path& destination) const noexcept;
      sqlite3* handle() const;
      std::mutex& mutex() const;

    private:
      sqlite3* database_ = nullptr;
      bool initialized_ = false;
      int schema_cookie_ = 0;
      mutable std::mutex mutex_;
  };
} // namespace Chess::Backend

#endif // ICHESS_BACKEND_SQLITE_DATABASE