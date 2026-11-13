#include "http_api.h"
#include "local_account_store.h"
#include "session_revocation_store.h"
#include "service_runtime.h"
#include "sqlite_game_application.h"
#include <cstdlib>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace
{
  volatile std::sig_atomic_t shutdown_requested = 0;

  extern "C" void request_shutdown(int)
  {
    shutdown_requested = 1;
  }

  std::string environment(const char* name, bool required)
  {
#ifdef _WIN32
    char* raw = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&raw, &length, name) != 0)
      throw std::runtime_error("Invalid runtime environment.");
    const std::unique_ptr<char, decltype(&std::free)> value(raw, &std::free);
    if (length > 4097 || (required && (raw == nullptr || length <= 1)))
      throw std::runtime_error("Invalid runtime environment.");
    return raw == nullptr ? std::string{} : std::string(raw);
#else
    const char* raw = std::getenv(name);
    if (raw == nullptr)
    {
      if (required) throw std::runtime_error("Invalid runtime environment.");
      return {};
    }
    const std::string value(raw);
    if (value.size() > 4096 || (required && value.empty()))
      throw std::runtime_error("Invalid runtime environment.");
    return value;
#endif
  }

  bool parse_boolean(const std::string& value)
  {
    if (value.empty() || value == "true") return true;
    if (value == "false") return false;
    throw std::runtime_error("Invalid runtime environment.");
  }

  class SignalHandlers
  {
    public:
      SignalHandlers()
      {
        previous_interrupt_ = std::signal(SIGINT, request_shutdown);
        if (previous_interrupt_ == SIG_ERR) throw std::runtime_error("Signal initialization failed.");
        previous_terminate_ = std::signal(SIGTERM, request_shutdown);
        if (previous_terminate_ == SIG_ERR)
        {
          std::signal(SIGINT, previous_interrupt_);
          throw std::runtime_error("Signal initialization failed.");
        }
      }
      ~SignalHandlers()
      {
        std::signal(SIGINT, previous_interrupt_);
        std::signal(SIGTERM, previous_terminate_);
      }
    private:
      using Handler = void (*)(int);
      Handler previous_interrupt_ = SIG_DFL;
      Handler previous_terminate_ = SIG_DFL;
  };

  class IdempotencyMaintenanceWorker
  {
    public:
      explicit IdempotencyMaintenanceWorker(Chess::Backend::SqliteGameApplication& application)
        : thread_([this, &application] {
          while (!stop_.load(std::memory_order_acquire))
          {
            for (int second = 0; second < 60 && !stop_.load(std::memory_order_acquire); ++second)
              std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!stop_.load(std::memory_order_acquire) && !application.cleanup_expired_idempotency())
              std::clog << "{\"event\":\"idempotency_maintenance_failed\",\"failures\":"
                        << application.maintenance_failures() << "}\n";
          }
        })
      {}

      ~IdempotencyMaintenanceWorker()
      {
        stop_.store(true, std::memory_order_release);
        if (thread_.joinable()) thread_.join();
      }

      IdempotencyMaintenanceWorker(const IdempotencyMaintenanceWorker&) = delete;
      IdempotencyMaintenanceWorker& operator=(const IdempotencyMaintenanceWorker&) = delete;

    private:
      std::atomic<bool> stop_{false};
      std::thread thread_;
  };

  class TimedGameExpiryWorker
  {
    public:
      explicit TimedGameExpiryWorker(Chess::Backend::SqliteGameApplication& application)
        : thread_([this, &application] {
          while (!stop_.load(std::memory_order_acquire))
          {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (stop_.load(std::memory_order_acquire)) break;
            std::size_t expired = 0;
            if (!application.expire_timed_games(64, expired))
              std::clog << "{\"event\":\"timed_game_expiry_failed\"}\n";
          }
        })
      {}

      ~TimedGameExpiryWorker()
      {
        stop_.store(true, std::memory_order_release);
        if (thread_.joinable()) thread_.join();
      }

      TimedGameExpiryWorker(const TimedGameExpiryWorker&) = delete;
      TimedGameExpiryWorker& operator=(const TimedGameExpiryWorker&) = delete;

    private:
      std::atomic<bool> stop_{false};
      std::thread thread_;
  };
}

int main()
{
  try
  {
    using namespace Chess::Backend;
    const auto port_text = environment("ICHESS_PORT", false);
    const auto session_path = environment("ICHESS_SESSION_FILE", false);
    const auto database_path = environment("ICHESS_DB_PATH", true);
    const auto revocation_path = environment("ICHESS_REVOCATION_DB_PATH", true);
    const auto account_path = environment("ICHESS_ACCOUNT_DB_PATH", true);
    const bool allow_registration = parse_boolean(environment("ICHESS_ALLOW_REGISTRATION", false));
    const auto minimum_free_storage = environment("ICHESS_MIN_FREE_STORAGE_BYTES", false);
    const ServiceRuntimeConfig config{environment("ICHESS_PUBLIC_ORIGIN", true),
        session_path.empty() ? std::filesystem::path{}
          : std::filesystem::path(std::u8string(session_path.begin(), session_path.end())),
      std::filesystem::path(std::u8string(database_path.begin(), database_path.end())),
      std::filesystem::path(std::u8string(revocation_path.begin(), revocation_path.end())),
      std::filesystem::path(std::u8string(account_path.begin(), account_path.end())),
        port_text.empty() ? 8080 : parse_service_port(port_text),
        minimum_free_storage.empty() ? 64ULL * 1024 * 1024 : parse_storage_reserve(minimum_free_storage)};
    validate_runtime_config(config);
    const auto revocation_utf8 = config.revocation_database_file.u8string();
    const std::string revocation_filename(revocation_utf8.begin(), revocation_utf8.end());
    auto revocations = std::make_shared<SqliteSessionRevocationStore>(revocation_filename);
    const auto account_utf8 = config.account_database_file.u8string();
    const std::string account_filename(account_utf8.begin(), account_utf8.end());
    SqliteLocalAccountStore accounts(account_filename);
    SessionAuthenticator authenticator({config.public_origin}, std::chrono::system_clock::now,
                                       std::chrono::steady_clock::now, revocations);
    if (!config.session_file.empty()) load_preview_sessions(config.session_file, authenticator);
    const auto database_utf8 = config.database_file.u8string();
    SqliteDatabase database(std::string(database_utf8.begin(), database_utf8.end()));
    SqliteGameApplication application(database);
    if (!application.is_ready() || !revocations->is_ready() || !accounts.is_ready()
      || !storage_reserve_available(config.database_file.parent_path(), config.minimum_free_storage_bytes))
      throw std::runtime_error("Storage initialization failed.");
    ServiceRateLimiter limiter;
    RuntimeObserver observer([](std::string event) { std::clog << event << '\n'; });
    httplib::Server server;
    HttpApiConfig api_config;
    api_config.allow_registration = allow_registration;
    register_http_api(server, authenticator, application, api_config,
      [revocations, &accounts, config] {
        return revocations->is_ready() && accounts.is_ready()
            && storage_reserve_available(config.database_file.parent_path(), config.minimum_free_storage_bytes);
      },
      &accounts, &authenticator);
    configure_service_runtime(server, authenticator, limiter, &observer);
    if (!server.bind_to_port("127.0.0.1", config.port)) throw std::runtime_error("Listener initialization failed.");
    SignalHandlers signals;
    ShutdownMonitor monitor(server, shutdown_requested);
    IdempotencyMaintenanceWorker maintenance(application);
    TimedGameExpiryWorker timed_expiry(application);
    return server.listen_after_bind() ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  catch (...)
  {
    std::cerr << "Private preview service startup or runtime failed.\n";
    return EXIT_FAILURE;
  }
}