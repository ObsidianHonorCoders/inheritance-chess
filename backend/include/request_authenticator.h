#ifndef ICHESS_BACKEND_REQUEST_AUTHENTICATOR
#define ICHESS_BACKEND_REQUEST_AUTHENTICATOR

#include "api_types.h"
#include <httplib.h>
#include <string>

namespace Chess::Backend
{
  struct AuthenticationResult
  {
      bool authenticated = false;
      int failure_status = 401;
      Principal principal;
      std::string csrf_token;
  };

  class IRequestAuthenticator
  {
    public:
      virtual ~IRequestAuthenticator() = default;

      // Implementations must verify the server-side session and, for writes,
      // CSRF token and Origin. Never trust an identity header by itself.
      virtual AuthenticationResult authenticate(const httplib::Request& request,
                                                bool state_changing) const = 0;
  };
} // namespace Chess::Backend

#endif // ICHESS_BACKEND_REQUEST_AUTHENTICATOR
