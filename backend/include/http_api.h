#ifndef ICHESS_BACKEND_HTTP_API
#define ICHESS_BACKEND_HTTP_API

#include "game_application.h"
#include "request_authenticator.h"
#include <httplib.h>
#include <cstddef>
#include <ctime>

namespace Chess::Backend
{
  struct HttpApiConfig
  {
      std::size_t max_request_body_bytes = 16 * 1024;
      std::size_t max_history_page_size = 200;
      std::time_t read_timeout_seconds = 5;
      std::time_t write_timeout_seconds = 10;
  };

  void register_http_api(httplib::Server& server,
                         const IRequestAuthenticator& authenticator,
                         IGameApplication& application,
                         const HttpApiConfig& config = {});
} // namespace Chess::Backend

#endif // ICHESS_BACKEND_HTTP_API