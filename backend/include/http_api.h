#ifndef ICHESS_BACKEND_HTTP_API
#define ICHESS_BACKEND_HTTP_API

#include "game_application.h"
#include "request_authenticator.h"
#include "local_account_store.h"
#include "session_authenticator.h"
#include <httplib.h>
#include <cstddef>
#include <ctime>
#include <functional>

namespace Chess::Backend
{
  struct HttpApiConfig
  {
      std::size_t max_request_body_bytes = 16 * 1024;
      std::size_t max_history_page_size = 200;
      std::time_t read_timeout_seconds = 5;
      std::time_t write_timeout_seconds = 10;
      bool allow_registration = true;
  };

  // The route handlers keep references to authenticator and application; both must outlive the server.
  void register_http_api(httplib::Server& server,
                         const IRequestAuthenticator& authenticator,
                         IGameApplication& application,
                         const HttpApiConfig& config = {},
                         std::function<bool()> additional_readiness = {},
                         ILocalAccountStore* accounts = nullptr,
                         ILoginSessionManager* sessions = nullptr);
} // namespace Chess::Backend

#endif // ICHESS_BACKEND_HTTP_API
