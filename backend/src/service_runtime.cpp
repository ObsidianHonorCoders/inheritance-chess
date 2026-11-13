#include "service_runtime.h"
#include "execution_budget.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <charconv>
#include <memory>
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

    struct RequestScope
    {
        std::unique_ptr<ExecutionBudget> budget;
        std::chrono::steady_clock::time_point start;
        std::uint64_t id = 0;
        bool active = false;
        bool password_slot = false;
    };

    thread_local RequestScope request_scope;
    std::atomic<std::uint64_t> next_request_id{1};

    std::string correlation_id(std::uint64_t value)
    {
      std::array<char, 16> buffer{};
      buffer.fill('0');
      std::array<char, 16> digits{};
      const auto result = std::to_chars(digits.data(), digits.data() + digits.size(), value, 16);
      const auto length = static_cast<std::size_t>(result.ptr - digits.data());
      std::copy_n(digits.data(), length, buffer.data() + buffer.size() - length);
      return std::string(buffer.data(), buffer.size());
    }

    RuntimeMethod normalized_method(const std::string& method)
    {
      if (method == "GET") return RuntimeMethod::Get;
      if (method == "HEAD") return RuntimeMethod::Head;
      if (method == "POST") return RuntimeMethod::Post;
      if (method == "PUT") return RuntimeMethod::Put;
      if (method == "PATCH") return RuntimeMethod::Patch;
      if (method == "DELETE") return RuntimeMethod::Delete;
      if (method == "OPTIONS") return RuntimeMethod::Options;
      return RuntimeMethod::Other;
    }

    RuntimeRoute normalized_route(const std::string& path)
    {
      if (path == "/api/v1/games" || path.compare(0, 14, "/api/v1/games/") == 0)
        return RuntimeRoute::Games;
      if (path == "/api/v1/session") return RuntimeRoute::Session;
      if (path == "/api/v1/health/live" || path == "/api/v1/health/ready") return RuntimeRoute::Health;
      return RuntimeRoute::Other;
    }

    const char* method_name(RuntimeMethod method)
    {
      switch (method)
      {
        case RuntimeMethod::Get: return "GET";
        case RuntimeMethod::Head: return "HEAD";
        case RuntimeMethod::Post: return "POST";
        case RuntimeMethod::Put: return "PUT";
        case RuntimeMethod::Patch: return "PATCH";
        case RuntimeMethod::Delete: return "DELETE";
        case RuntimeMethod::Options: return "OPTIONS";
        default: return "OTHER";
      }
    }

    const char* route_name(RuntimeRoute route)
    {
      switch (route)
      {
        case RuntimeRoute::Games: return "games";
        case RuntimeRoute::Session: return "session";
        case RuntimeRoute::Health: return "health";
        default: return "other";
      }
    }

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
      response.set_header("Connection", "close");
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

  std::uint64_t parse_storage_reserve(std::string_view text)
  {
    std::uint64_t bytes = 0;
    if (text.empty() || text.size() > 20)
      invalid_configuration();
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), bytes);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
      invalid_configuration();
    return bytes;
  }

  bool storage_reserve_available(const std::filesystem::path& path,
                                 std::uint64_t minimum_free_bytes) noexcept
  {
    try
    {
      std::error_code error;
      const auto space = std::filesystem::space(path, error);
      return !error && space.available >= minimum_free_bytes;
    }
    catch (...) { return false; }
  }

  void validate_runtime_config(const ServiceRuntimeConfig& config)
  {
    namespace fs = std::filesystem;
    if (!SessionAuthenticator::valid_public_origin(config.public_origin) || config.port < 1 || config.port > 65535
      || (!config.session_file.empty()
        && (!config.session_file.is_absolute() || config.session_file.filename().empty()))
      || !config.database_file.is_absolute()
      || !config.revocation_database_file.is_absolute()
      || !config.account_database_file.is_absolute() || config.database_file.filename().empty()
      || config.revocation_database_file.filename().empty() || config.account_database_file.filename().empty()
      || (!config.session_file.empty()
        && config.database_file.lexically_normal() == config.session_file.lexically_normal())
      || config.database_file.lexically_normal() == config.revocation_database_file.lexically_normal()
      || config.database_file.lexically_normal() == config.account_database_file.lexically_normal()
      || (!config.session_file.empty()
        && config.session_file.lexically_normal() == config.revocation_database_file.lexically_normal())
      || (!config.session_file.empty()
        && config.session_file.lexically_normal() == config.account_database_file.lexically_normal())
      || config.revocation_database_file.lexically_normal() == config.account_database_file.lexically_normal()
      || config.database_file.parent_path().lexically_normal()
        != config.revocation_database_file.parent_path().lexically_normal()
      || config.database_file.parent_path().lexically_normal()
        != config.account_database_file.parent_path().lexically_normal()) invalid_configuration();
    const auto database = config.database_file.lexically_normal();
    const auto revocations = config.revocation_database_file.lexically_normal();
    const auto accounts = config.account_database_file.lexically_normal();
    if (database.filename() == "." || database.filename() == ".."
      || revocations.filename() == "." || revocations.filename() == ".."
      || accounts.filename() == "." || accounts.filename() == ".."
      || database.string().find(':', 2) != std::string::npos
      || revocations.string().find(':', 2) != std::string::npos
      || accounts.string().find(':', 2) != std::string::npos) invalid_configuration();
#ifdef _WIN32
    if (database.native().find_first_of(L"?*\"<>|") != std::wstring::npos
      || revocations.native().find_first_of(L"?*\"<>|") != std::wstring::npos
      || accounts.native().find_first_of(L"?*\"<>|") != std::wstring::npos
        || database.native().compare(0, 2, L"\\\\") == 0
      || revocations.native().compare(0, 2, L"\\\\") == 0
      || accounts.native().compare(0, 2, L"\\\\") == 0
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
    current.clear();
    for (const auto& part : revocations)
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
    current.clear();
    for (const auto& part : accounts)
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
        || (fs::exists(database) && !fs::is_regular_file(database))
        || (fs::exists(revocations) && !fs::is_regular_file(revocations))
        || (fs::exists(accounts) && !fs::is_regular_file(accounts))) invalid_configuration();
    if (fs::exists(database) && fs::exists(config.session_file) && fs::equivalent(database, config.session_file))
      invalid_configuration();
    if (fs::exists(revocations) && fs::exists(config.session_file)
      && fs::equivalent(revocations, config.session_file)) invalid_configuration();
    if (fs::exists(accounts) && fs::exists(config.session_file)
      && fs::equivalent(accounts, config.session_file)) invalid_configuration();
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
        || config_.create_requests == 0 || config_.create_requests > 1000000
        || config_.join_requests == 0 || config_.join_requests > 1000000
        || config_.mutation_requests == 0 || config_.mutation_requests > 1000000
        || config_.login_requests == 0 || config_.login_requests > 1000000
        || config_.registration_requests == 0 || config_.registration_requests > 1000000
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

  bool ServiceRateLimiter::allow_operation(const Principal& principal, std::string_view method, std::string_view path)
  {
    if (!SessionAuthenticator::valid_principal(principal.id)) return false;
    if (method != "POST") return true;
    constexpr std::string_view games = "/api/v1/games";
    if (path == games)
      return allow(operations_[0], principal.id, config_.max_principals, config_.create_requests);
    if (path.size() <= games.size() || path.substr(0, games.size() + 1) != "/api/v1/games/") return true;
    const auto suffix = path.substr(games.size() + 1);
    if (suffix.find('/') == std::string_view::npos)
      return allow(operations_[1], principal.id, config_.max_principals, config_.join_requests);
    return allow(operations_[2], principal.id, config_.max_principals, config_.mutation_requests);
  }

  bool ServiceRateLimiter::allow_auth_attempt(const std::string& peer, bool registration)
  {
    const auto index = registration ? 1U : 0U;
    const auto requests = registration ? config_.registration_requests : config_.login_requests;
    return allow(auth_attempts_[index], peer, config_.max_peers, requests);
  }

  std::pair<std::size_t, std::size_t> ServiceRateLimiter::state_size() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return {peers_.size(), principals_.size()};
  }

  std::chrono::seconds ServiceRateLimiter::retry_after() const { return config_.window; }

  RuntimeObserver::RuntimeObserver(Sink sink) : sink_(std::move(sink)) {}

  void RuntimeObserver::record(const RuntimeEvent& event) noexcept
  {
    try
    {
      {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        ++metrics_.total;
        const auto bucket = event.status >= 100 && event.status < 600 ? event.status / 100 : 0;
        ++metrics_.by_status[static_cast<std::size_t>(bucket)];
        metrics_.total_duration_ms += event.duration_ms;
      }
      if (sink_)
      {
        const auto text = nlohmann::json{{"requestId", correlation_id(event.request_id)},
          {"method", method_name(event.method)}, {"route", route_name(event.route)},
          {"status", event.status}, {"durationMs", event.duration_ms}}.dump();
        std::lock_guard<std::mutex> lock(sink_mutex_);
        sink_(text);
      }
    }
    catch (...) {}
  }

  RuntimeMetrics RuntimeObserver::snapshot() const
  {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
  }

  PasswordWorkGate::PasswordWorkGate(std::size_t maximum) : maximum_(maximum)
  {
    if (maximum_ == 0 || maximum_ > 4) invalid_configuration();
  }

  bool PasswordWorkGate::try_acquire() noexcept
  {
    auto active = active_.load(std::memory_order_relaxed);
    while (active < maximum_)
    {
      if (active_.compare_exchange_weak(active, active + 1,
          std::memory_order_acquire, std::memory_order_relaxed)) return true;
    }
    return false;
  }

  void PasswordWorkGate::release() noexcept
  {
    auto active = active_.load(std::memory_order_relaxed);
    while (active != 0 && !active_.compare_exchange_weak(active, active - 1,
        std::memory_order_release, std::memory_order_relaxed)) {}
  }

  void configure_service_runtime(httplib::Server& server, const SessionAuthenticator& authenticator,
                                 ServiceRateLimiter& limiter, RuntimeObserver* observer, RuntimeOptions options)
  {
    if (options.execution_timeout < std::chrono::milliseconds(1)
        || options.execution_timeout > std::chrono::milliseconds(60000)
        || options.max_concurrent_password_hashes == 0 || options.max_concurrent_password_hashes > 4)
      invalid_configuration();
    server.new_task_queue = [] { return new httplib::ThreadPool(4, 64); };
    server.set_keep_alive_max_count(16);
    server.set_keep_alive_timeout(2);
    server.set_read_timeout(5);
    server.set_write_timeout(10);
    server.set_payload_max_length(16 * 1024);
    auto password_gate = std::make_shared<PasswordWorkGate>(options.max_concurrent_password_hashes);
    server.set_pre_routing_handler([&authenticator, &limiter, options, password_gate](const httplib::Request& request,
                                                            httplib::Response& response) {
      request_scope.budget.reset();
      if (request_scope.password_slot) password_gate->release();
      request_scope.password_slot = false;
      request_scope.active = false;
      request_scope.start = std::chrono::steady_clock::now();
      request_scope.id = next_request_id.fetch_add(1, std::memory_order_relaxed);
      request_scope.budget = std::make_unique<ExecutionBudget>(options.execution_timeout);
      request_scope.active = true;
      response.headers.erase("X-Request-ID");
      response.set_header("X-Request-ID", correlation_id(request_scope.id));
      if (!limiter.allow_peer(request.remote_addr))
      {
        rate_limited(response, limiter.retry_after());
        return httplib::Server::HandlerResponse::Handled;
      }
        if (request.method == "POST" && (request.path == "/api/v1/auth/login"
          || request.path == "/api/v1/auth/password")
          && !limiter.allow_auth_attempt(request.remote_addr, false))
      {
        rate_limited(response, limiter.retry_after());
        return httplib::Server::HandlerResponse::Handled;
      }
      if (request.method == "POST" && request.path == "/api/v1/auth/register"
          && !limiter.allow_auth_attempt(request.remote_addr, true))
      {
        rate_limited(response, limiter.retry_after());
        return httplib::Server::HandlerResponse::Handled;
      }
      if (request.method == "POST"
          && (request.path == "/api/v1/auth/login" || request.path == "/api/v1/auth/register"
            || request.path == "/api/v1/auth/password"))
      {
        if (!password_gate->try_acquire())
        {
          rate_limited(response, std::chrono::seconds(1));
          return httplib::Server::HandlerResponse::Handled;
        }
        request_scope.password_slot = true;
      }
      if (request.path == "/api/v1" || request.path.compare(0, 8, "/api/v1/") == 0)
      {
        const auto principal = authenticator.principal_for(request);
        if (principal && (!limiter.allow_principal(*principal)
              || !limiter.allow_operation(*principal, request.method, request.path)))
        {
          rate_limited(response, limiter.retry_after());
          return httplib::Server::HandlerResponse::Handled;
        }
      }
      return httplib::Server::HandlerResponse::Unhandled;
    });
    server.set_post_routing_handler([](const httplib::Request&, httplib::Response& response) {
      if (!request_scope.active)
      {
        request_scope.start = std::chrono::steady_clock::now();
        request_scope.id = next_request_id.fetch_add(1, std::memory_order_relaxed);
        request_scope.active = true;
      }
      response.headers.erase("X-Request-ID");
      response.set_header("X-Request-ID", correlation_id(request_scope.id));
    });
    server.set_logger([observer, password_gate](const httplib::Request& request, const httplib::Response& response) {
      const auto active = request_scope.active;
      const auto id = active ? request_scope.id : next_request_id.fetch_add(1, std::memory_order_relaxed);
      const auto duration = active ? std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - request_scope.start).count() : 0.0;
      request_scope.budget.reset();
      if (request_scope.password_slot) password_gate->release();
      request_scope.password_slot = false;
      request_scope.active = false;
      if (observer)
        observer->record({id, normalized_method(request.method), normalized_route(request.path),
                          response.status, duration});
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