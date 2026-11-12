#ifndef ICHESS_BACKEND_SERVICE_RUNTIME
#define ICHESS_BACKEND_SERVICE_RUNTIME

#include "session_authenticator.h"
#include <atomic>
#include <csignal>
#include <filesystem>
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
  };

  class ServiceRateLimiter
  {
    public:
      using Clock = std::function<std::chrono::steady_clock::time_point()>;
      explicit ServiceRateLimiter(RateLimitConfig config = {}, Clock clock = std::chrono::steady_clock::now);
      bool allow_peer(const std::string& peer);
      bool allow_principal(const Principal& principal);
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
  };

  void configure_service_runtime(httplib::Server& server, const SessionAuthenticator& authenticator,
                                 ServiceRateLimiter& limiter);

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