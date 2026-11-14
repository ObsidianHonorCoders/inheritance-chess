#ifndef ICHESS_BACKEND_SQLITE_BACKUP
#define ICHESS_BACKEND_SQLITE_BACKUP

#include <filesystem>
#include <mutex>
#include <sqlite3.h>

namespace Chess::Backend
{
  bool sqlite_backup_to_file(sqlite3* source, std::mutex& source_mutex,
                             const std::filesystem::path& destination) noexcept;
}

#endif