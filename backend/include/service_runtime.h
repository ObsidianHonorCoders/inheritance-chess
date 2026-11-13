#ifndef ICHESS_BACKEND_SERVICE_RUNTIME
#define ICHESS_BACKEND_SERVICE_RUNTIME

#include "session_authenticator.h"
#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace Chess::Backend
{
  struct PreviewSession
  {
      Principal principal;
      std::string token;
      std::int64_t expires;
  };

  std::vector<PreviewSession> parse_preview_sessions(const std::string& json, std::int64_t now,
                                                    std::size_t max_sessions = 1024);
  void load_preview_sessions(const std::filesystem::path& path, SessionAuthenticator& authenticator);

  struct ServiceRuntimeConfig
  {
      std::string public_origin;
      std::filesystem::path session_file;
      std::filesystem::path database_file;
      std::filesystem::path revocation_database_file;
      std::filesystem::path account_database_file;
      int port = 8080;
  };

  int parse_service_port(std::string_view text);
  void validate_runtime_config(const ServiceRuntimeConfig& config);

  struct RateLimitConfig
  {
      std::size_t max_peers = 2048;
      std::size_t max_principals = 1024;
      std::size_t peer_requests = 240;
      std::size_t principal_requests = 120;
      std::chrono::seconds window{60};
      std::size_t create_requests = 10;
      std::size_t join_requests = 20;
      std::size_t mutation_requests = 60;
      std::size_t login_requests = 20;
      std::size_t registration_requests = 5;
  };

  class ServiceRateLimiter
  {
    public:
      using Clock = std::function<std::chrono::steady_clock::time_point()>;
      explicit ServiceRateLimiter(RateLimitConfig config = {}, Clock clock = std::chrono::steady_clock::now);
      bool allow_peer(const std::string& peer);
      bool allow_principal(const Principal& principal);
      bool allow_operation(const Principal& principal, std::string_view method, std::string_view path);
      bool allow_auth_attempt(const std::string& peer, bool registration);
      std::pair<std::size_t, std::size_t> state_size() const;
      std::chrono::seconds retry_after() const;

    private:
      struct Bucket
      {
          std::size_t requests;
          std::chrono::steady_clock::time_point reset;
      };
      using Buckets = std::unordered_map<std::string, Bucket>;
      bool allow(Buckets& buckets, const std::string& key, std::size_t capacity, std::size_t requests);
      void prune(Buckets& buckets) const;
      RateLimitConfig config_;
      Clock clock_;
      mutable std::mutex mutex_;
      Buckets peers_;
      Buckets principals_;
      std::array<Buckets, 3> operations_;
      std::array<Buckets, 2> auth_attempts_;
  };

  enum class RuntimeMethod { Other, Get, Head, Post, Put, Patch, Delete, Options };
  enum class RuntimeRoute { Other, Games, Session, Health };

  struct RuntimeEvent
  {
      std::uint64_t request_id = 0;
      RuntimeMethod method = RuntimeMethod::Other;
      RuntimeRoute route = RuntimeRoute::Other;
      int status = 0;
      double duration_ms = 0;
  };

  struct RuntimeMetrics
  {
      std::uint64_t total = 0;
      std::array<std::uint64_t, 6> by_status{};
      double total_duration_ms = 0;
  };

  class RuntimeObserver
  {
    public:
      using Sink = std::function<void(std::string)>;
      explicit RuntimeObserver(Sink sink = {});
      void record(const RuntimeEvent& event) noexcept;
      RuntimeMetrics snapshot() const;

    private:
      Sink sink_;
      mutable std::mutex metrics_mutex_;
      std::mutex sink_mutex_;
      RuntimeMetrics metrics_;
  };

  struct RuntimeOptions
  {
      std::chrono::milliseconds execution_timeout{2000};
      std::size_t max_concurrent_password_hashes = 2;
  };

  class PasswordWorkGate final
  {
    public:
      explicit PasswordWorkGate(std::size_t maximum);
      bool try_acquire() noexcept;
      void release() noexcept;

    private:
      const std::size_t maximum_;
      std::atomic<std::size_t> active_{0};
  };

  void configure_service_runtime(httplib::Server& server, const SessionAuthenticator& authenticator,
                                 ServiceRateLimiter& limiter, RuntimeObserver* observer = nullptr,
                                 RuntimeOptions options = {});

  class ShutdownMonitor
  {
    public:
      ShutdownMonitor(httplib::Server& server, const volatile std::sig_atomic_t& signal);
      ~ShutdownMonitor();
      ShutdownMonitor(const ShutdownMonitor&) = delete;
      ShutdownMonitor& operator=(const ShutdownMonitor&) = delete;

    private:
      std::atomic<bool> done_{false};
      std::thread thread_;
  };
}

#endif