#include "session_authenticator.h"
#include <gtest/gtest.h>
#include <atomic>
#include <thread>

namespace
{
  using namespace Chess::Backend;

  class SessionAuthenticatorTest : public testing::Test
  {
    protected:
      std::chrono::system_clock::time_point wall{std::chrono::seconds(1700000000)};
      std::chrono::steady_clock::time_point monotonic{std::chrono::seconds(100)};
      SessionAuthenticator auth{{"https://chess.example.test", 2, std::chrono::seconds(3600)},
          [this] { return wall; }, [this] { return monotonic; }};

      static httplib::Request request(const std::string& token)
      {
        httplib::Request result;
        result.set_header("Cookie", "__Host-ichess_session=" + token);
        return result;
      }
  };

  TEST_F(SessionAuthenticatorTest, GeneratesIndependentCredentialsAndSecureCookie)
  {
    const auto first = auth.create_session(Principal{"alice"});
    const auto second = auth.create_session(Principal{"bob"});
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_TRUE(SessionAuthenticator::valid_token(first->token));
    EXPECT_TRUE(SessionAuthenticator::valid_token(first->csrf_token));
    EXPECT_NE(first->token, second->token);
    EXPECT_NE(first->token, first->csrf_token);
    EXPECT_NE(first->set_cookie.find("; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age=3600"), std::string::npos);
    EXPECT_EQ(auth.authenticate(request(first->token), false).principal.id, "alice");
    EXPECT_EQ(auth.authenticate(request(second->token), false).principal.id, "bob");
  }

  TEST_F(SessionAuthenticatorTest, RequiresExactOriginAndCsrfForWrites)
  {
    const auto session = auth.create_session(Principal{"alice"});
    ASSERT_TRUE(session);
    auto write = request(session->token);
    EXPECT_EQ(auth.authenticate(write, true).failure_status, 403);
    write.set_header("Origin", "https://chess.example.test");
    write.set_header("X-CSRF-Token", std::string(64, '0'));
    EXPECT_FALSE(auth.authenticate(write, true).authenticated);
    write.headers.erase("X-CSRF-Token");
    write.set_header("X-CSRF-Token", session->csrf_token);
    EXPECT_TRUE(auth.authenticate(write, true).authenticated);
    write.headers.erase("Origin");
    for (const auto* origin : {"null", "http://chess.example.test", "https://chess.example.test/",
                              "https://chess.example.test.evil", "https://chess.example.test:443"})
    {
      write.headers.erase("Origin");
      write.set_header("Origin", origin);
      EXPECT_FALSE(auth.authenticate(write, true).authenticated);
    }
    write.headers.erase("Origin");
    write.set_header("Origin", "https://chess.example.test");
    write.headers.emplace("Origin", "https://chess.example.test");
    EXPECT_FALSE(auth.authenticate(write, true).authenticated);
    write.headers.erase("Origin");
    write.set_header("Origin", "https://chess.example.test");
    write.headers.emplace("X-CSRF-Token", session->csrf_token);
    EXPECT_FALSE(auth.authenticate(write, true).authenticated);
  }

  TEST_F(SessionAuthenticatorTest, RejectsAmbiguousAndMalformedCookiesAndIdentityHeaders)
  {
    const auto session = auth.create_session(Principal{"alice"});
    ASSERT_TRUE(session);
    httplib::Request forged;
    forged.set_header("X-Principal", "alice");
    forged.set_header("Authorization", "Bearer " + session->token);
    EXPECT_FALSE(auth.authenticate(forged, false).authenticated);
    EXPECT_FALSE(auth.authenticate(request(std::string(64, '0')), false).authenticated);
    const auto cookie = "__Host-ichess_session=" + session->token;
    for (const auto& value : {cookie + "; " + cookie, cookie + ";", cookie + "; broken",
        "__Host-ichess_session=\"" + session->token + "\"", cookie + "; x=bad value",
        "ichess_session=" + session->token, cookie + "; x=1; x=2"})
    {
      httplib::Request invalid;
      invalid.set_header("Cookie", value);
      EXPECT_FALSE(auth.authenticate(invalid, false).authenticated);
    }
    auto duplicate = request(session->token);
    duplicate.headers.emplace("Cookie", cookie);
    EXPECT_FALSE(auth.authenticate(duplicate, false).authenticated);
    auto valid = request(session->token);
    valid.headers.erase("Cookie");
    valid.set_header("Cookie", "other=okay; " + cookie);
    EXPECT_TRUE(auth.authenticate(valid, false).authenticated);
  }

  TEST_F(SessionAuthenticatorTest, ExpiresAtBoundaryAndCannotExtendWithClockRollback)
  {
    const auto session = auth.create_session(Principal{"alice"});
    ASSERT_TRUE(session);
    wall += std::chrono::seconds(3599);
    EXPECT_TRUE(auth.authenticate(request(session->token), false).authenticated);
    wall += std::chrono::seconds(1);
    EXPECT_FALSE(auth.authenticate(request(session->token), false).authenticated);
    const auto next = auth.create_session(Principal{"alice"});
    ASSERT_TRUE(next);
    wall -= std::chrono::hours(2);
    monotonic += std::chrono::hours(1);
    EXPECT_FALSE(auth.authenticate(request(next->token), false).authenticated);
  }

  TEST_F(SessionAuthenticatorTest, ImportsAreBoundedExpiringAndRevocable)
  {
    constexpr std::int64_t now = 1700000000;
    const std::string token(64, 'a');
    EXPECT_FALSE(auth.import_session(Principal{"alice"}, token, 0));
    EXPECT_FALSE(auth.import_session(Principal{"alice"}, token, now));
    EXPECT_FALSE(auth.import_session(Principal{"alice"}, token, now + 86401));
    EXPECT_FALSE(auth.import_session(Principal{}, token, now + 100));
    EXPECT_FALSE(auth.import_session(Principal{"alice"}, "short", now + 100));
    ASSERT_TRUE(auth.import_session(Principal{"alice"}, token, now + 100));
    EXPECT_FALSE(auth.import_session(Principal{"bob"}, token, now + 100));
    EXPECT_EQ(auth.principal_for(request(token))->id, "alice");
    ASSERT_TRUE(auth.create_session(Principal{"bob"}));
    EXPECT_FALSE(auth.create_session(Principal{"charlie"}));
    EXPECT_FALSE(auth.import_session(Principal{"charlie"}, std::string(64, 'b'), now + 100));
    EXPECT_EQ(auth.session_count(), 2U);
    EXPECT_TRUE(auth.revoke_session(token));
    EXPECT_FALSE(auth.revoke_session(token));
    EXPECT_FALSE(auth.authenticate(request(token), false).authenticated);
    EXPECT_TRUE(auth.create_session(Principal{"charlie"}));
    wall += std::chrono::hours(1);
    EXPECT_EQ(auth.session_count(), 0U);
  }

  TEST(SessionConfigurationTest, RejectsUnsafeOriginsAndBounds)
  {
    for (const auto* origin : {"http://chess.test", "https://user@chess.test", "https://chess.test/",
                              "https://chess.test:0", "https://chess.test:65536", "https://a..test",
                              "https://-a.test", "https://chess.test?x", "https://chess.test#x"})
      EXPECT_FALSE(SessionAuthenticator::valid_public_origin(origin));
    EXPECT_TRUE(SessionAuthenticator::valid_public_origin("https://chess.test:8443"));
    EXPECT_THROW(SessionAuthenticator((SessionAuthConfig{"https://chess.test", 0})), std::invalid_argument);
    EXPECT_THROW(SessionAuthenticator((SessionAuthConfig{"https://chess.test", 4097})), std::invalid_argument);
    EXPECT_THROW(SessionAuthenticator((SessionAuthConfig{"https://chess.test", 1, std::chrono::hours(25)})),
                 std::invalid_argument);
  }

  TEST(SessionConcurrencyTest, CreationAndRevocationStayWithinCapacity)
  {
    SessionAuthenticator auth({"https://chess.test", 8});
    std::atomic<int> created{0};
    std::vector<std::thread> threads;
    for (int index = 0; index < 4; ++index)
      threads.emplace_back([&] {
        for (int attempt = 0; attempt < 8; ++attempt)
          if (auth.create_session(Principal{"alice"})) ++created;
      });
    for (auto& thread : threads) thread.join();
    EXPECT_EQ(created.load(), 8);
    EXPECT_EQ(auth.session_count(), 8U);
  }
}