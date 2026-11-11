#include "http_api.h"
#include "local_adapters.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
  int configured_port()
  {
    const char* configured = std::getenv("ICHESS_BACKEND_PORT");
    if (configured == nullptr || *configured == '\0')
    {
      return 8080;
    }

    char* end = nullptr;
    const long parsed = std::strtol(configured, &end, 10);
    if (end == configured || *end != '\0' || parsed < 1 || parsed > 65535)
    {
      throw std::runtime_error("ICHESS_BACKEND_PORT must be an integer from 1 to 65535.");
    }
    return static_cast<int>(parsed);
  }
} // namespace

int main()
{
  try
  {
    const int port = configured_port();
    Chess::Backend::InMemoryGameApplication application;
    Chess::Backend::LocalSessionAuthenticator authenticator({
      "http://localhost:5173",
      "http://127.0.0.1:5173"
    });
    httplib::Server server;

    Chess::Backend::register_local_session_routes(server, authenticator);
    Chess::Backend::register_http_api(server, authenticator, application);
    if (!server.bind_to_port("127.0.0.1", port))
    {
      std::cerr << "Could not bind the local backend to 127.0.0.1:" << port << '\n';
      return 1;
    }

    std::cout << "Local development backend listening at http://127.0.0.1:" << port
              << " (loopback only; ephemeral sessions and games)\n";
    return server.listen_after_bind() ? 0 : 1;
  }
  catch (const std::exception& exception)
  {
    std::cerr << "Backend startup failed: " << exception.what() << '\n';
    return 1;
  }
}
