#include "session_authenticator.h"
#include "session_revocation_store.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <set>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif

namespace Chess::Backend
{
  namespace
  {
    constexpr std::string_view cookie_name = "__Host-ichess_session";

    bool constant_time_equal(std::string_view left, std::string_view right)
    {
      if (left.size() != 64 || right.size() != 64) return false;
      volatile unsigned int difference = 0;
      for (std::size_t index = 0; index < 64; ++index)
      {
        difference = difference | (static_cast<unsigned char>(left[index])
                                ^ static_cast<unsigned char>(right[index]));
      }
      return difference == 0;
    }

    std::string random_token()
    {
      std::array<unsigned char, 32> bytes{};
#ifdef _WIN32
      if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()),
                          BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
        throw std::runtime_error("Session initialization failed.");
#else
      std::ifstream random("/dev/urandom", std::ios::binary);
      if (!random.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("Session initialization failed.");
#endif
      constexpr char hex[] = "0123456789abcdef";
      std::string token;
      token.reserve(64);
      for (const auto value : bytes)
      {
        token.push_back(hex[value >> 4]);
        token.push_back(hex[value & 15]);
      }
      return token;
    }

    bool ascii_alnum(char value)
    {
      return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z')
          || (value >= '0' && value <= '9');
    }

    std::optional<std::string> cookie_token(const httplib::Request& request)
    {
      if (request.get_header_value_count("Cookie") != 1) return std::nullopt;
      const auto header = request.get_header_value("Cookie");
      if (header.empty() || header.size() > 8192) return std::nullopt;
      std::string_view remaining(header);
      std::set<std::string_view> names;
      std::optional<std::string> token;
      while (!remaining.empty())
      {
        const auto separator = remaining.find(';');
        auto pair = remaining.substr(0, separator);
        while (!pair.empty() && pair.front() == ' ') pair.remove_prefix(1);
        const auto equal = pair.find('=');
        if (equal == std::string_view::npos || equal == 0) return std::nullopt;
        const auto name = pair.substr(0, equal);
        const auto value = pair.substr(equal + 1);
        for (const char character : name)
        {
          if (!ascii_alnum(character) && std::string_view("!#$%&'*+-.^_`|~").find(character)
                  == std::string_view::npos) return std::nullopt;
        }
        for (const unsigned char character : value)
        {
          if (!(character == 0x21 || (character >= 0x23 && character <= 0x2b)
              || (character >= 0x2d && character <= 0x3a)
              || (character >= 0x3c && character <= 0x5b)
              || (character >= 0x5d && character <= 0x7e))) return std::nullopt;
        }
        if (!names.insert(name).second) return std::nullopt;
        if (name == cookie_name)
        {
          if (!SessionAuthenticator::valid_token(value)) return std::nullopt;
          token = std::string(value);
        }
        if (separator == std::string_view::npos) break;
        remaining.remove_prefix(separator + 1);
        if (remaining.empty()) return std::nullopt;
      }
      return token;
    }

    std::int64_t epoch(std::chrono::system_clock::time_point value)
    {
      return std::chrono::duration_cast<std::chrono::seconds>(value.time_since_epoch()).count();
    }
  }

  bool SessionAuthenticator::valid_public_origin(std::string_view origin)
  {
    if (origin.size() > 300 || origin.substr(0, 8) != "https://") return false;
    auto authority = origin.substr(8);
    const auto colon = authority.find(':');
    const auto host = authority.substr(0, colon);
    if (host.empty() || host.size() > 253) return false;
    std::size_t label_length = 0;
    char previous = '.';
    for (const char character : host)
    {
      if (character == '.')
      {
        if (label_length == 0 || previous == '-') return false;
        label_length = 0;
      }
      else
      {
        if (!(character >= 'a' && character <= 'z') && !(character >= '0' && character <= '9')
            && character != '-') return false;
        if ((label_length == 0 && character == '-') || ++label_length > 63) return false;
      }
      previous = character;
    }
    if (label_length == 0 || previous == '-') return false;
    if (colon != std::string_view::npos)
    {
      const auto port_text = authority.substr(colon + 1);
      unsigned int port = 0;
      const auto parsed = std::from_chars(port_text.data(), port_text.data() + port_text.size(), port);
      if (port_text.empty() || parsed.ec != std::errc{} || parsed.ptr != port_text.data() + port_text.size()
          || port == 0 || port > 65535) return false;
    }
    return true;
  }

  bool SessionAuthenticator::valid_principal(std::string_view principal)
  {
    return !principal.empty() && principal.size() <= 128
        && std::all_of(principal.begin(), principal.end(), [](char value) {
          return ascii_alnum(value) || std::string_view("._:@-").find(value) != std::string_view::npos;
        });
  }

  bool SessionAuthenticator::valid_token(std::string_view token)
  {
    return token.size() == 64 && std::all_of(token.begin(), token.end(), [](char value) {
      return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f')
          || (value >= 'A' && value <= 'F');
    });
  }

  SessionAuthenticator::SessionAuthenticator(SessionAuthConfig config, WallClock wall_clock,
                                             MonotonicClock monotonic_clock,
                                             std::shared_ptr<ISessionRevocationStore> store)
      : config_(std::move(config)), wall_clock_(std::move(wall_clock)),
        monotonic_clock_(std::move(monotonic_clock)), store_(std::move(store))
  {
    if (!valid_public_origin(config_.public_origin) || config_.max_sessions == 0
        || config_.max_sessions > 4096 || config_.lifetime.count() <= 0
        || config_.lifetime > std::chrono::hours(24) || !wall_clock_ || !monotonic_clock_)
      throw std::invalid_argument("Invalid session configuration.");
    sessions_.reserve(config_.max_sessions);
  }

  void SessionAuthenticator::prune_locked() const
  {
    const auto now = epoch(wall_clock_());
    const auto monotonic_now = monotonic_clock_();
    sessions_.erase(std::remove_if(sessions_.begin(), sessions_.end(), [&](const Session& session) {
      return session.expires <= now || session.deadline <= monotonic_now;
    }), sessions_.end());
  }

  const SessionAuthenticator::Session* SessionAuthenticator::find_locked(std::string_view token) const
  {
    const Session* found = nullptr;
    for (const auto& session : sessions_)
    {
      if (constant_time_equal(session.token, token)) found = &session;
    }
    return found;
  }

  std::optional<SessionCredentials> SessionAuthenticator::create_session(const Principal& principal)
  {
    if (!valid_principal(principal.id)) return std::nullopt;
    std::lock_guard<std::mutex> lock(mutex_);
    prune_locked();
    if (sessions_.size() >= config_.max_sessions) return std::nullopt;
    auto token = random_token();
    if (find_locked(token) != nullptr || revoked(token)) return std::nullopt;
    auto csrf = random_token();
    const auto expires = epoch(wall_clock_()) + config_.lifetime.count();
    sessions_.push_back(Session{principal, token, csrf, expires, monotonic_clock_() + config_.lifetime});
    return SessionCredentials{token, csrf, expires, std::string(cookie_name) + "=" + token
        + "; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age=" + std::to_string(config_.lifetime.count())};
  }

  bool SessionAuthenticator::import_session(const Principal& principal, const std::string& token,
                                            std::int64_t expires)
  {
    if (!valid_principal(principal.id) || !valid_token(token)) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    prune_locked();
    const auto now = epoch(wall_clock_());
    if (expires <= now || expires > now + 86400 || sessions_.size() >= config_.max_sessions
      || find_locked(token) != nullptr || revoked(token)) return false;
    sessions_.push_back(Session{principal, token, random_token(), expires,
                                monotonic_clock_() + std::chrono::seconds(expires - now)});
    return true;
  }

  bool SessionAuthenticator::revoke_session(const std::string& token)
  {
    if (!valid_token(token)) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    prune_locked();
    const auto* found = find_locked(token);
    if (found == nullptr) return false;
    if (store_)
    {
      try
      {
        store_->revoke(token, found->expires);
      }
      catch (...)
      {
        return false;
      }
    }
    sessions_.erase(std::remove_if(sessions_.begin(), sessions_.end(), [&](const Session& session) {
      return constant_time_equal(session.token, token);
    }), sessions_.end());
    return true;
  }

  bool SessionAuthenticator::revoked(std::string_view token) const
  {
    if (!store_) return false;
    try
    {
      return store_->is_revoked(token, epoch(wall_clock_()));
    }
    catch (...)
    {
      return true;
    }
  }

  std::size_t SessionAuthenticator::session_count() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    prune_locked();
    return sessions_.size();
  }

  std::optional<Principal> SessionAuthenticator::principal_for(const httplib::Request& request) const
  {
    const auto token = cookie_token(request);
    if (!token) return std::nullopt;
    std::lock_guard<std::mutex> lock(mutex_);
    prune_locked();
    const auto* session = find_locked(*token);
    if (session == nullptr || revoked(*token)) return std::nullopt;
    return session->principal;
  }

  AuthenticationResult SessionAuthenticator::authenticate(const httplib::Request& request, bool state_changing) const
  {
    const auto token = cookie_token(request);
    if (!token) return {};
    std::lock_guard<std::mutex> lock(mutex_);
    prune_locked();
    const auto* session = find_locked(*token);
    if (session == nullptr || revoked(*token)) return {};
    if (state_changing)
    {
      if (request.get_header_value_count("Origin") != 1
          || request.get_header_value("Origin") != config_.public_origin
          || request.get_header_value_count("X-CSRF-Token") != 1
          || !constant_time_equal(request.get_header_value("X-CSRF-Token"), session->csrf_token))
        return AuthenticationResult{false, 403, {}, {}};
    }
    return AuthenticationResult{true, 401, session->principal, session->csrf_token};
  }
}