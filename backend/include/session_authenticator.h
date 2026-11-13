#ifndef ICHESS_BACKEND_SESSION_AUTHENTICATOR
#define ICHESS_BACKEND_SESSION_AUTHENTICATOR

#include "request_authenticator.h"
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <vector>

namespace Chess::Backend
{
  class ISessionRevocationStore;

  struct SessionAuthConfig
  {
      std::string public_origin;
      std::size_t max_sessions = 1024;
      std::chrono::seconds lifetime{3600};
  };

  struct SessionCredentials
  {
      std::string token;
      std::string csrf_token;
      std::int64_t expires = 0;
      std::string set_cookie;
  };

  class ILoginSessionManager
  {
    public:
      virtual ~ILoginSessionManager() = default;
      virtual bool accepts_login_origin(const httplib::Request& request) const = 0;
      virtual std::optional<SessionCredentials> issue_session(const Principal& principal) = 0;
      virtual bool revoke_principal_sessions(const Principal& principal) = 0;
      virtual bool block_principal(const Principal& principal) = 0;
      virtual void unblock_principal(const Principal& principal) noexcept = 0;
      virtual bool revoke_request_session(const httplib::Request& request) = 0;
  };

  class SessionAuthenticator final : public IRequestAuthenticator, public ILoginSessionManager
  {
    public:
      using WallClock = std::function<std::chrono::system_clock::time_point()>;
      using MonotonicClock = std::function<std::chrono::steady_clock::time_point()>;

      explicit SessionAuthenticator(SessionAuthConfig config,
          WallClock wall_clock = std::chrono::system_clock::now,
          MonotonicClock monotonic_clock = std::chrono::steady_clock::now,
          std::shared_ptr<ISessionRevocationStore> store = {});

      std::optional<SessionCredentials> create_session(const Principal& principal);
      bool accepts_login_origin(const httplib::Request& request) const override;
      std::optional<SessionCredentials> issue_session(const Principal& principal) override;
      bool revoke_principal_sessions(const Principal& principal) override;
      bool block_principal(const Principal& principal) override;
      void unblock_principal(const Principal& principal) noexcept override;
      bool import_session(const Principal& principal, const std::string& token, std::int64_t expires);
      bool revoke_session(const std::string& token);
      bool revoke_request_session(const httplib::Request& request) override;
      std::size_t session_count() const;
      std::optional<Principal> principal_for(const httplib::Request& request) const;
      AuthenticationResult authenticate(const httplib::Request& request, bool state_changing) const override;

      static bool valid_public_origin(std::string_view origin);
      static bool valid_principal(std::string_view principal);
      static bool valid_token(std::string_view token);

    private:
      struct Session
      {
          Principal principal;
          std::string token;
          std::string csrf_token;
          std::int64_t expires;
          std::chrono::steady_clock::time_point deadline;
      };

      void prune_locked() const;
      const Session* find_locked(std::string_view token) const;
      bool revoked(std::string_view token) const;
      SessionAuthConfig config_;
      WallClock wall_clock_;
      MonotonicClock monotonic_clock_;
      std::shared_ptr<ISessionRevocationStore> store_;
      mutable std::mutex mutex_;
      mutable std::vector<Session> sessions_;
      std::vector<Principal> blocked_principals_;
  };
}

#endif