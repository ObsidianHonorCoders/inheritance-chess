#include "service_runtime.h"
#include <nlohmann/json.hpp>
#include <array>
#include <charconv>
#include <set>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <aclapi.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/vfs.h>
#endif
#endif

namespace Chess::Backend
{
  namespace
  {
    [[noreturn]] void invalid_configuration()
    {
      throw std::runtime_error("Invalid private preview configuration.");
    }

    constexpr std::size_t max_file_bytes = 64 * 1024;

#ifdef _WIN32
    struct FileHandle
    {
        HANDLE value;
        ~FileHandle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    };

    struct SecurityDescriptor
    {
        PSECURITY_DESCRIPTOR value = nullptr;
        ~SecurityDescriptor() { if (value != nullptr) LocalFree(value); }
    };

    bool restricted_file(HANDLE file)
    {
      HANDLE raw_token = INVALID_HANDLE_VALUE;
      if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &raw_token)) return false;
      FileHandle token{raw_token};
      alignas(TOKEN_USER) std::array<unsigned char, sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE> user_storage{};
      DWORD required = 0;
      if (!GetTokenInformation(token.value, TokenUser, user_storage.data(),
                               static_cast<DWORD>(user_storage.size()), &required)) return false;
      const auto* user = reinterpret_cast<const TOKEN_USER*>(user_storage.data());
      alignas(DWORD) std::array<unsigned char, SECURITY_MAX_SID_SIZE> system_storage{};
      alignas(DWORD) std::array<unsigned char, SECURITY_MAX_SID_SIZE> admin_storage{};
      DWORD system_size = static_cast<DWORD>(system_storage.size());
      DWORD admin_size = static_cast<DWORD>(admin_storage.size());
      if (!CreateWellKnownSid(WinLocalSystemSid, nullptr, system_storage.data(), &system_size)
          || !CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, admin_storage.data(), &admin_size))
        return false;
      SecurityDescriptor descriptor;
      PACL dacl = nullptr;
      PSID owner = nullptr;
      if (GetSecurityInfo(file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
                          &owner, nullptr, &dacl, nullptr, &descriptor.value) != ERROR_SUCCESS
          || dacl == nullptr || owner == nullptr || !EqualSid(owner, user->User.Sid)) return false;
      for (DWORD index = 0; index < dacl->AceCount; ++index)
      {
        void* raw_ace = nullptr;
        if (!GetAce(dacl, index, &raw_ace)) return false;
        const auto* header = static_cast<const ACE_HEADER*>(raw_ace);
        if ((header->AceFlags & INHERIT_ONLY_ACE) != 0) continue;
        if (header->AceType == ACCESS_DENIED_ACE_TYPE) continue;
        if (header->AceType != ACCESS_ALLOWED_ACE_TYPE) return false;
        const auto* ace = static_cast<const ACCESS_ALLOWED_ACE*>(raw_ace);
        auto* sid = const_cast<DWORD*>(&ace->SidStart);
        if (!EqualSid(sid, user->User.Sid) && !EqualSid(sid, system_storage.data())
            && !EqualSid(sid, admin_storage.data())) return false;
      }
      return true;
    }
#else
    struct FileHandle
    {
        int value;
        ~FileHandle() { if (value >= 0) close(value); }
    };
#endif

    std::string read_private_file(const std::filesystem::path& path)
    {
      if (!path.is_absolute() || path.filename().empty()) invalid_configuration();
#ifdef _WIN32
      FileHandle file{CreateFileW(path.c_str(), GENERIC_READ | READ_CONTROL, FILE_SHARE_READ,
          nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr)};
      if (file.value == INVALID_HANDLE_VALUE) invalid_configuration();
      BY_HANDLE_FILE_INFORMATION info{};
      if (!GetFileInformationByHandle(file.value, &info)
          || (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0
          || info.nFileSizeHigh != 0 || info.nFileSizeLow == 0 || info.nFileSizeLow > max_file_bytes
          || !restricted_file(file.value)) invalid_configuration();
#else
      FileHandle file{open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC)};
      struct stat info{};
      if (file.value < 0 || fstat(file.value, &info) != 0 || !S_ISREG(info.st_mode)
          || info.st_uid != geteuid() || (info.st_mode & 0077) != 0 || info.st_size <= 0
          || info.st_size > static_cast<off_t>(max_file_bytes)) invalid_configuration();
#endif
      std::string text;
      std::array<char, 4096> buffer{};
      while (true)
      {
#ifdef _WIN32
        DWORD count = 0;
        if (!ReadFile(file.value, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr))
          invalid_configuration();
#else
        const auto count = read(file.value, buffer.data(), buffer.size());
        if (count < 0) invalid_configuration();
#endif
        if (count == 0) break;
        if (text.size() + static_cast<std::size_t>(count) > max_file_bytes) invalid_configuration();
        text.append(buffer.data(), static_cast<std::size_t>(count));
      }
      return text;
    }

    void rate_limited(httplib::Response& response, std::chrono::seconds retry)
    {
      response.status = 429;
      response.set_header("Cache-Control", "no-store");
      response.set_header("Retry-After", std::to_string(retry.count()));
      response.set_header("X-Content-Type-Options", "nosniff");
      response.set_content(R"({"type":"about:blank","title":"Too Many Requests","status":429,"detail":"Request limit reached."})",
                           "application/problem+json");
    }
  }

  std::vector<PreviewSession> parse_preview_sessions(const std::string& text, std::int64_t now,
                                                    std::size_t max_sessions)
  {
    if (text.empty() || text.size() > max_file_bytes || now < 0 || max_sessions == 0 || max_sessions > 4096)
      invalid_configuration();
    try
    {
      std::vector<std::set<std::string>> keys(5);
      const auto parsed = nlohmann::json::parse(text, [&](int depth, nlohmann::json::parse_event_t event,
                                                        nlohmann::json& value) {
        if (depth < 0 || depth > 3) invalid_configuration();
        const auto slot = static_cast<std::size_t>(depth);
        if (event == nlohmann::json::parse_event_t::object_start) keys[slot + 1].clear();
        if (event == nlohmann::json::parse_event_t::key
            && !keys[slot].insert(value.get<std::string>()).second) invalid_configuration();
        return true;
      });
      if (!parsed.is_array() || parsed.empty() || parsed.size() > max_sessions) invalid_configuration();
      std::set<std::string> tokens;
      std::vector<PreviewSession> result;
      result.reserve(parsed.size());
      for (const auto& entry : parsed)
      {
        if (!entry.is_object() || entry.size() != 3 || !entry.contains("principal")
            || !entry.contains("token") || !entry.contains("expires")
            || !entry["principal"].is_string() || !entry["token"].is_string()
            || !entry["expires"].is_number_integer()) invalid_configuration();
        const auto& expiry = entry["expires"];
        if (expiry.is_number_unsigned() && expiry.get<std::uint64_t>() > 253402300799ULL)
          invalid_configuration();
        const auto expires = expiry.get<std::int64_t>();
        if (expires <= now || expires - now > 86400) invalid_configuration();
        auto principal = entry["principal"].get<std::string>();
        auto token = entry["token"].get<std::string>();
        if (!SessionAuthenticator::valid_principal(principal) || !SessionAuthenticator::valid_token(token)
            || !tokens.insert(token).second) invalid_configuration();
        result.push_back(PreviewSession{Principal{std::move(principal)}, std::move(token), expires});
      }
      return result;
    }
    catch (...)
    {
      invalid_configuration();
    }
  }

  void load_preview_sessions(const std::filesystem::path& path, SessionAuthenticator& authenticator)
  {
    const auto text = read_private_file(path);
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const auto sessions = parse_preview_sessions(text, now);
    for (const auto& session : sessions)
      if (!authenticator.import_session(session.principal, session.token, session.expires)) invalid_configuration();
  }

  int parse_service_port(std::string_view text)
  {
    int port = 0;
    if (text.empty() || text.size() > 5) invalid_configuration();
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), port);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || port < 1 || port > 65535)
      invalid_configuration();
    return port;
  }

  void validate_runtime_config(const ServiceRuntimeConfig& config)
  {
    namespace fs = std::filesystem;
    if (!SessionAuthenticator::valid_public_origin(config.public_origin) || config.port < 1 || config.port > 65535
        || !config.session_file.is_absolute() || !config.database_file.is_absolute()
        || config.session_file.filename().empty() || config.database_file.filename().empty()
        || config.database_file == config.session_file) invalid_configuration();
    const auto database = config.database_file.lexically_normal();
    if (database.filename() == "." || database.filename() == ".."
        || database.string().find(':', 2) != std::string::npos) invalid_configuration();
#ifdef _WIN32
    if (database.native().find_first_of(L"?*\"<>|") != std::wstring::npos
        || database.native().compare(0, 2, L"\\\\") == 0
        || GetDriveTypeW(database.root_path().c_str()) != DRIVE_FIXED) invalid_configuration();
#endif
    fs::path current;
    for (const auto& part : database)
    {
      current /= part;
      if (fs::exists(current))
      {
        if (fs::is_symlink(fs::symlink_status(current))) invalid_configuration();
#ifdef _WIN32
        const auto attributes = GetFileAttributesW(current.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
          invalid_configuration();
#endif
      }
    }
    if (!fs::is_directory(database.parent_path())
        || (fs::exists(database) && !fs::is_regular_file(database))) invalid_configuration();
    if (fs::exists(database) && fs::exists(config.session_file) && fs::equivalent(database, config.session_file))
      invalid_configuration();
#ifdef __linux__
    struct statfs info{};
  if (statfs(database.parent_path().c_str(), &info) != 0
    || (info.f_type != 0xef53 && info.f_type != 0x58465342
      && info.f_type != 0x9123683e && info.f_type != 0x2fc12fc1))
      invalid_configuration();
#endif
  }

  ServiceRateLimiter::ServiceRateLimiter(RateLimitConfig config, Clock clock)
      : config_(config), clock_(std::move(clock))
  {
    if (config_.max_peers == 0 || config_.max_peers > 65536 || config_.max_principals == 0
        || config_.max_principals > 65536 || config_.peer_requests == 0 || config_.peer_requests > 1000000
        || config_.principal_requests == 0 || config_.principal_requests > 1000000
        || config_.window.count() < 1 || config_.window > std::chrono::hours(1) || !clock_)
      invalid_configuration();
  }

  void ServiceRateLimiter::prune(Buckets& buckets) const
  {
    const auto now = clock_();
    for (auto entry = buckets.begin(); entry != buckets.end();)
    {
      if (entry->second.reset <= now) entry = buckets.erase(entry);
      else ++entry;
    }
  }

  bool ServiceRateLimiter::allow(Buckets& buckets, const std::string& key, std::size_t capacity,
                                std::size_t requests)
  {
    if (key.empty() || key.size() > 128) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    prune(buckets);
    auto entry = buckets.find(key);
    if (entry == buckets.end())
    {
      if (buckets.size() >= capacity) return false;
      entry = buckets.emplace(key, Bucket{0, clock_() + config_.window}).first;
    }
    if (entry->second.requests >= requests) return false;
    ++entry->second.requests;
    return true;
  }

  bool ServiceRateLimiter::allow_peer(const std::string& peer)
  {
    return allow(peers_, peer, config_.max_peers, config_.peer_requests);
  }

  bool ServiceRateLimiter::allow_principal(const Principal& principal)
  {
    if (!SessionAuthenticator::valid_principal(principal.id)) return false;
    return allow(principals_, principal.id, config_.max_principals, config_.principal_requests);
  }

  std::pair<std::size_t, std::size_t> ServiceRateLimiter::state_size() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return {peers_.size(), principals_.size()};
  }

  std::chrono::seconds ServiceRateLimiter::retry_after() const { return config_.window; }

  void configure_service_runtime(httplib::Server& server, const SessionAuthenticator& authenticator,
                                 ServiceRateLimiter& limiter)
  {
    server.new_task_queue = [] { return new httplib::ThreadPool(4, 64); };
    server.set_keep_alive_max_count(16);
    server.set_keep_alive_timeout(2);
    server.set_read_timeout(5);
    server.set_write_timeout(10);
    server.set_payload_max_length(16 * 1024);
    server.set_pre_routing_handler([&authenticator, &limiter](const httplib::Request& request,
                                                            httplib::Response& response) {
      if (!limiter.allow_peer(request.remote_addr))
      {
        rate_limited(response, limiter.retry_after());
        return httplib::Server::HandlerResponse::Handled;
      }
      if (request.path == "/api/v1" || request.path.compare(0, 8, "/api/v1/") == 0)
      {
        const auto principal = authenticator.principal_for(request);
        if (principal && !limiter.allow_principal(*principal))
        {
          rate_limited(response, limiter.retry_after());
          return httplib::Server::HandlerResponse::Handled;
        }
      }
      return httplib::Server::HandlerResponse::Unhandled;
    });
  }

  ShutdownMonitor::ShutdownMonitor(httplib::Server& server, const volatile std::sig_atomic_t& signal)
      : thread_([this, &server, &signal] {
        while (!done_.load(std::memory_order_acquire))
        {
          if (signal != 0 && server.is_running())
          {
            server.stop();
            return;
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
      })
  {}

  ShutdownMonitor::~ShutdownMonitor()
  {
    done_.store(true, std::memory_order_release);
    if (thread_.joinable()) thread_.join();
  }
}