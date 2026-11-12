#include "http_api.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace
{
  using namespace Chess::Backend;
  using Json = nlohmann::json;

  constexpr const char* kValidationCookie = "__Host-ichess_session=validation-session";
  constexpr const char* kValidationCsrf = "0123456789abcdef0123456789abcdef";
  constexpr const char* kValidationOrigin = "https://chess.example.test";
  constexpr const char* kValidationGameId = "abcdefghijklmnopqrstuv";

  class ValidationAuthenticator final : public IRequestAuthenticator
  {
    public:
      AuthenticationResult authenticate(const httplib::Request& request, bool state_changing) const override
      {
        if (request.get_header_value("Cookie") != kValidationCookie)
        {
          return {};
        }
        if (state_changing
            && (request.get_header_value("Origin") != kValidationOrigin
                || request.get_header_value("X-CSRF-Token") != kValidationCsrf))
        {
          return AuthenticationResult{false, 403, {}, {}};
        }
        return AuthenticationResult{true, 401, Principal{"validation-principal"}, kValidationCsrf};
      }
  };

  class ValidationApplication final : public IGameApplication
  {
    public:
      mutable std::atomic<int> calls{0};

      bool is_ready() const override
      {
        ++calls;
        return true;
      }

      Result<GameSnapshot> create_game(const Principal&, const CreateGameCommand&, const std::string&) override
      {
        return snapshot();
      }

      Result<GameSnapshot> get_game(const Principal&, const std::string&) const override
      {
        return snapshot();
      }

      Result<GameSnapshot> join_game(const Principal&, const std::string&, std::uint64_t,
                                     const std::string&) override
      {
        return snapshot();
      }

      Result<LegalMoveList> get_legal_moves(const Principal&, const std::string&) const override
      {
        ++calls;
        return LegalMoveList{};
      }

      Result<MovePage> get_move_history(const Principal&, const std::string&, std::uint64_t,
                                       std::uint64_t) const override
      {
        ++calls;
        return MovePage{};
      }

      Result<GameSnapshot> submit_move(const Principal&, const std::string&, const MoveCommand&,
                                       const std::string&) override
      {
        return snapshot();
      }

      Result<GameSnapshot> resign(const Principal&, const std::string&, std::uint64_t,
                                  const std::string&) override
      {
        return snapshot();
      }

      Result<GameSnapshot> offer_draw(const Principal&, const std::string&, std::uint64_t,
                                      const std::string&) override
      {
        return snapshot();
      }

      Result<GameSnapshot> accept_draw(const Principal&, const std::string&, std::uint64_t,
                                       const std::string&) override
      {
        return snapshot();
      }

    private:
      GameSnapshot snapshot() const
      {
        ++calls;
        GameSnapshot game;
        game.game_id = kValidationGameId;
        game.version = 2;
        game.status = "ongoing";
        return game;
      }
  };

  class HttpValidationTest : public ::testing::Test
  {
    protected:
      void SetUp() override
      {
        register_http_api(server_, authenticator_, application_);
        const int port = server_.bind_to_any_port("127.0.0.1");
        ASSERT_GT(port, 0);
        server_thread_ = std::thread([this] { server_.listen_after_bind(); });
        for (int attempt = 0; attempt < 100 && !server_.is_running(); ++attempt)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        ASSERT_TRUE(server_.is_running());
        client_ = std::make_unique<httplib::Client>("127.0.0.1", port);
        client_->set_connection_timeout(1, 0);
        client_->set_read_timeout(2, 0);
      }

      void TearDown() override
      {
        server_.stop();
        if (server_thread_.joinable())
        {
          server_thread_.join();
        }
      }

      static std::string game_path(const std::string& suffix = {})
      {
        return std::string("/api/v1/games/") + kValidationGameId + suffix;
      }

      static httplib::Headers headers()
      {
        return {
          {"Cookie", kValidationCookie},
          {"Origin", kValidationOrigin},
          {"X-CSRF-Token", kValidationCsrf},
          {"Idempotency-Key", "validation-request-key"},
          {"If-Match", "\"1\""},
          {"Content-Type", "application/json"}
        };
      }

      httplib::Result send(const std::string& method, const std::string& path,
                           httplib::Headers request_headers, const std::string& body = {})
      {
        httplib::Request request;
        request.method = method;
        request.path = path;
        request.headers = std::move(request_headers);
        request.body = body;
        return client_->send(request);
      }

      static void expect_safe_bad_request(const httplib::Response& response)
      {
        EXPECT_EQ(response.status, 400);
        EXPECT_EQ(response.get_header_value("Content-Type").find("application/problem+json"), 0u);
        EXPECT_EQ(response.get_header_value("Cache-Control"), "private, no-store");
        EXPECT_EQ(response.get_header_value("X-Content-Type-Options"), "nosniff");
        EXPECT_EQ(response.get_header_value("Referrer-Policy"), "no-referrer");
        const Json problem = Json::parse(response.body);
        EXPECT_EQ(problem.at("status"), 400);
        EXPECT_EQ(problem.at("title"), "Bad Request");
        EXPECT_EQ(response.body.find("private-marker"), std::string::npos);
        EXPECT_EQ(response.body.find(kValidationCookie), std::string::npos);
      }

      ValidationAuthenticator authenticator_;
      ValidationApplication application_;
      httplib::Server server_;
      std::thread server_thread_;
      std::unique_ptr<httplib::Client> client_;
  };

  class HttpActionBodyValidationTest : public HttpValidationTest,
                                       public ::testing::WithParamInterface<const char*>
  {};

  TEST_P(HttpActionBodyValidationTest, AcceptsOnlyEmptyBodyOrEmptyJsonObject)
  {
    for (const std::string body : {"", "{}", " \r\n { } \t"})
    {
      SCOPED_TRACE(body);
      const auto response = send("POST", game_path(GetParam()), headers(), body);
      ASSERT_TRUE(response);
      EXPECT_EQ(response->status, 200);
      EXPECT_EQ(response->get_header_value("ETag"), "\"2\"");
    }
    EXPECT_EQ(application_.calls.load(), 3);
  }

  TEST_P(HttpActionBodyValidationTest, AcceptsEmptyBodyWithoutContentType)
  {
    auto request_headers = headers();
    request_headers.erase("Content-Type");
    const auto response = send("POST", game_path(GetParam()), request_headers);
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);
    EXPECT_EQ(application_.calls.load(), 1);
  }

  TEST_P(HttpActionBodyValidationTest, RejectsMalformedNonObjectAndNonemptyBodiesBeforeApplication)
  {
    for (const std::string body : {
           "{private-marker", "{} private-marker", " ", "[]", "null", "true", "42", "\"private-marker\"",
           "{\"private-marker\":1}", "{\"uci\":\"e2e4\"}", "{\"version\":1}",
           "{\"private-marker\":1,\"private-marker\":2}"})
    {
      SCOPED_TRACE(body);
      const auto response = send("POST", game_path(GetParam()), headers(), body);
      ASSERT_TRUE(response);
      expect_safe_bad_request(*response);
      EXPECT_EQ(application_.calls.load(), 0);
    }
  }

  TEST_P(HttpActionBodyValidationTest, RejectsNonJsonContentTypeBeforeApplication)
  {
    auto request_headers = headers();
    request_headers.erase("Content-Type");
    request_headers.emplace("Content-Type", "text/plain");
    const auto response = send("POST", game_path(GetParam()), request_headers, "{}");
    ASSERT_TRUE(response);
    expect_safe_bad_request(*response);
    EXPECT_EQ(application_.calls.load(), 0);
  }

  INSTANTIATE_TEST_SUITE_P(GameActions, HttpActionBodyValidationTest,
                          ::testing::Values("", "/resign", "/draw-offers", "/draw-offers/accept"));

  class HttpDuplicateHeaderValidationTest : public HttpValidationTest,
                                            public ::testing::WithParamInterface<const char*>
  {};

  TEST_P(HttpDuplicateHeaderValidationTest, RejectsIdenticalAndMixedCaseDuplicatesOnWrites)
  {
    for (const bool mixed_case : {false, true})
    {
      auto request_headers = headers();
      std::string duplicate_name = GetParam();
      const std::string value = request_headers.find(duplicate_name)->second;
      if (mixed_case)
      {
        std::transform(duplicate_name.begin(), duplicate_name.end(), duplicate_name.begin(), [](unsigned char character) {
          return static_cast<char>(std::tolower(character));
        });
      }
      request_headers.emplace(duplicate_name, value);
      for (const std::string suffix : {"", "/resign", "/draw-offers", "/draw-offers/accept", "/moves"})
      {
        SCOPED_TRACE(std::string(GetParam()) + suffix);
        const auto response = send("POST", game_path(suffix), request_headers, "{}");
        ASSERT_TRUE(response);
        expect_safe_bad_request(*response);
        EXPECT_EQ(Json::parse(response->body).at("code"), "invalid_header");
      }
      const auto response = send("POST", "/api/v1/games", request_headers, "{}");
      ASSERT_TRUE(response);
      expect_safe_bad_request(*response);
    }
    EXPECT_EQ(application_.calls.load(), 0);
  }

  TEST_P(HttpDuplicateHeaderValidationTest, RejectsDuplicatesOnReadsAndSession)
  {
    auto request_headers = headers();
    request_headers.emplace(GetParam(), request_headers.find(GetParam())->second);
    for (const std::string path : {game_path(), game_path("/moves"), game_path("/legal-moves"),
                                   std::string("/api/v1/session"), std::string("/api/v1/health/live"),
                                   std::string("/api/v1/health/ready")})
    {
      SCOPED_TRACE(path);
      const auto response = send("GET", path, request_headers);
      ASSERT_TRUE(response);
      expect_safe_bad_request(*response);
    }
    EXPECT_EQ(application_.calls.load(), 0);
  }

  INSTANTIATE_TEST_SUITE_P(SecurityHeaders, HttpDuplicateHeaderValidationTest,
                          ::testing::Values("Cookie", "Origin", "X-CSRF-Token", "Idempotency-Key",
                                            "If-Match", "Content-Type"));

  TEST_F(HttpValidationTest, RejectsDuplicateNumericQueriesBeforeApplication)
  {
    for (const std::string query : {
           "?limit=1&limit=1", "?limit=1&limit=2", "?limit=1&limit=private-marker", "?limit=&limit=1",
           "?afterPly=0&afterPly=0", "?afterPly=0&afterPly=1", "?afterPly=0&afterPly=private-marker",
           "?afterPly=&afterPly=0", "?limit=1&%6cimit=2", "?afterPly=0&%61fterPly=1"})
    {
      SCOPED_TRACE(query);
      const auto response = send("GET", game_path("/moves") + query, headers());
      ASSERT_TRUE(response);
      expect_safe_bad_request(*response);
      EXPECT_EQ(Json::parse(response->body).at("code"), "invalid_query");
      EXPECT_EQ(application_.calls.load(), 0);
    }
  }

  TEST_F(HttpValidationTest, AcceptsAbsentAndSingleNumericQueries)
  {
    for (const std::string query : {"", "?limit=1", "?afterPly=0", "?afterPly=2&limit=3"})
    {
      const auto response = send("GET", game_path("/moves") + query, headers());
      ASSERT_TRUE(response);
      EXPECT_EQ(response->status, 200);
    }
    EXPECT_EQ(application_.calls.load(), 4);
  }

  TEST_F(HttpValidationTest, RejectsUnauthenticatedWritesBeforeBodyAndHeaderValidation)
  {
    auto request_headers = headers();
    request_headers.erase("Cookie");
    request_headers.erase("If-Match");
    request_headers.erase("Idempotency-Key");
    request_headers.emplace("Content-Type", "text/private-marker");
    for (const std::string path : {std::string("/api/v1/games"), game_path(), game_path("/resign"),
                                   game_path("/draw-offers"), game_path("/draw-offers/accept"), game_path("/moves")})
    {
      SCOPED_TRACE(path);
      const auto response = send("POST", path, request_headers, "{private-marker");
      ASSERT_TRUE(response);
      EXPECT_EQ(response->status, 401);
      EXPECT_EQ(Json::parse(response->body).at("code"), "unauthenticated");
    }
    EXPECT_EQ(application_.calls.load(), 0);
  }
}