#include "sqlite_backup.h"
#include <chrono>
#include <memory>
#include <string>
#include <thread>

namespace Chess::Backend
{
  namespace
  {
    std::string utf8_path(const std::filesystem::path& path)
    {
      const auto value = path.u8string();
      return std::string(value.begin(), value.end());
    }

    struct DatabaseCloser
    {
      void operator()(sqlite3* database) const { if (database != nullptr) sqlite3_close_v2(database); }
    };

    struct BackupCloser
    {
      void operator()(sqlite3_backup* backup) const { if (backup != nullptr) sqlite3_backup_finish(backup); }
    };

    struct PartialFileRemover
    {
        std::filesystem::path path;
        bool armed = false;
        bool keep = false;
        ~PartialFileRemover()
        {
          if (armed && !keep)
          {
            std::error_code error;
            std::filesystem::remove(path, error);
          }
        }
    };
  }

  bool sqlite_backup_to_file(sqlite3* source, std::mutex& source_mutex,
                             const std::filesystem::path& destination) noexcept
  {
    try
    {
      if (source == nullptr || !destination.is_absolute() || destination.filename().empty()) return false;
      std::error_code error;
      const auto normalized = destination.lexically_normal();
      if (!std::filesystem::is_directory(normalized.parent_path(), error) || error
          || std::filesystem::exists(normalized, error) || error)
        return false;

      std::lock_guard<std::mutex> lock(source_mutex);
      const char* source_name = sqlite3_db_filename(source, "main");
      if (source_name == nullptr) return false;
      std::u8string source_utf8;
      for (const char* character = source_name;
           *character != '\0'; ++character)
        source_utf8.push_back(static_cast<char8_t>(*character));
      const auto source_path = std::filesystem::absolute(
        std::filesystem::path(source_utf8), error).lexically_normal();
      if (error || source_path == normalized) return false;
      if (std::filesystem::exists(source_path, error) && !error
          && std::filesystem::exists(normalized, error) && !error
          && std::filesystem::equivalent(source_path, normalized, error)) return false;

      sqlite3* raw_destination = nullptr;
      const auto destination_name = utf8_path(normalized);
      PartialFileRemover remove_partial{normalized};
      const int opened = sqlite3_open_v2(destination_name.c_str(), &raw_destination,
          SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_EXCLUSIVE | SQLITE_OPEN_FULLMUTEX, nullptr);
      const std::unique_ptr<sqlite3, DatabaseCloser> target(raw_destination);
      if (opened != SQLITE_OK || target == nullptr) return false;
      remove_partial.armed = true;
      if (sqlite3_busy_timeout(target.get(), 1000) != SQLITE_OK) return false;
      sqlite3_backup* raw_backup = sqlite3_backup_init(target.get(), "main", source, "main");
      std::unique_ptr<sqlite3_backup, BackupCloser> backup(raw_backup);
      if (backup == nullptr) return false;

      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
      int result = SQLITE_OK;
      do
      {
        result = sqlite3_backup_step(backup.get(), 256);
        if (result == SQLITE_BUSY || result == SQLITE_LOCKED)
        {
          if (std::chrono::steady_clock::now() >= deadline) break;
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      } while (result == SQLITE_OK || result == SQLITE_BUSY || result == SQLITE_LOCKED);
      const int finish = sqlite3_backup_finish(backup.get());
      backup.release();
      if (result == SQLITE_DONE && finish == SQLITE_OK)
      {
        sqlite3* verification = nullptr;
        const int checked = sqlite3_open_v2(destination_name.c_str(), &verification, SQLITE_OPEN_READONLY, nullptr);
        const std::unique_ptr<sqlite3, DatabaseCloser> readback(verification);
        if (checked == SQLITE_OK && readback != nullptr)
        {
          remove_partial.keep = true;
          return true;
        }
        return false;
      }
      return false;
    }
    catch (...) { return false; }
  }
}