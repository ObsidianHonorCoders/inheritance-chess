#include "http_api.h"
#include "service_runtime.h"
#include "sqlite_game_application.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

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
}

int main()
{
  try
  {
    using namespace Chess::Backend;
    const auto port_text = environment("ICHESS_PORT", false);
    const auto session_path = environment("ICHESS_SESSION_FILE", true);
    const auto database_path = environment("ICHESS_DB_PATH", true);
    const ServiceRuntimeConfig config{environment("ICHESS_PUBLIC_ORIGIN", true),
      std::filesystem::path(std::u8string(session_path.begin(), session_path.end())),
      std::filesystem::path(std::u8string(database_path.begin(), database_path.end())),
        port_text.empty() ? 8080 : parse_service_port(port_text)};
    validate_runtime_config(config);
    SessionAuthenticator authenticator({config.public_origin});
    load_preview_sessions(config.session_file, authenticator);
    const auto database_utf8 = config.database_file.u8string();
    SqliteDatabase database(std::string(database_utf8.begin(), database_utf8.end()));
    SqliteGameApplication application(database);
    if (!application.is_ready()) throw std::runtime_error("Storage initialization failed.");
    ServiceRateLimiter limiter;
    httplib::Server server;
    register_http_api(server, authenticator, application);
    configure_service_runtime(server, authenticator, limiter);
    if (!server.bind_to_port("127.0.0.1", config.port)) throw std::runtime_error("Listener initialization failed.");
    SignalHandlers signals;
    ShutdownMonitor monitor(server, shutdown_requested);
    return server.listen_after_bind() ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  catch (...)
  {
    std::cerr << "Private preview service startup or runtime failed.\n";
    return EXIT_FAILURE;
  }
}