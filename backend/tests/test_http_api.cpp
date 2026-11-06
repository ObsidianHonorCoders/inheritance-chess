#include "http_api.h"

#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <memory>
#include <thread>

namespace
{
  using namespace Chess::Backend;
  using Json = nlohmann::json;

  constexpr const char* kSessionCookie = "__Host-ichess_session=test-session";
  constexpr const char* kCsrfToken = "0123456789abcdef0123456789abcdef";
  constexpr const char* kOrigin = "https://chess.example.test";
  constexpr const char* kGameId = "abcdefghijklmnopqrstuv";

  class TestAuthenticator final : public IRequestAuthenticator
  {
    public:
      AuthenticationResult authenticate(const httplib::Request& request, bool state_changing) const override
      {
        if (request.get_header_value("Cookie") != kSessionCookie)
        {
          return {};
        }
        if (state_changing
            && (request.get_header_value("X-CSRF-Token") != kCsrfToken
                || request.get_header_value("Origin") != kOrigin))
        {
          return AuthenticationResult{false, 403, {}, {}};
        }
        return AuthenticationResult{true, 401, Principal{"test-principal"}, kCsrfToken};
      }
  };

  class TestGameApplication final : public IGameApplication
  {
    public:
      bool ready = true;
      int create_calls = 0;
      int move_calls = 0;
      std::uint64_t last_expected_version = 0;
      std::string last_idempotency_key;

      bool is_ready() const override { return ready; }

      Result<GameSnapshot> create_game(const Principal&, const CreateGameCommand&,
                                       const std::string& idempotency_key) override
      {
        ++create_calls;
        last_idempotency_key = idempotency_key;
        return snapshot();
      }

      Result<GameSnapshot> get_game(const Principal&, const std::string&) const override
      {
        return snapshot();
      }

      Result<GameSnapshot> join_game(const Principal&, const std::string&, std::uint64_t version,
                                     const std::string& idempotency_key) override
      {
        last_expected_version = version;
        last_idempotency_key = idempotency_key;
        return snapshot();
      }

      Result<LegalMoveList> get_legal_moves(const Principal&, const std::string&) const override
      {
        return LegalMoveList{2, {{"e2e4", "e4"}}};
      }

      Result<MovePage> get_move_history(const Principal&, const std::string&,
                                        std::uint64_t, std::uint64_t) const override
      {
        return MovePage{{MoveRecord{1, "e2e4", "e4"}}, std::nullopt};
      }

      Result<GameSnapshot> submit_move(const Principal&, const std::string&, const MoveCommand& command,
                                       const std::string& idempotency_key) override
      {
        ++move_calls;
        last_expected_version = command.expected_version;
        last_idempotency_key = idempotency_key;
        return snapshot();
      }

      Result<GameSnapshot> resign(const Principal&, const std::string&, std::uint64_t version,
                                  const std::string& idempotency_key) override
      {
        last_expected_version = version;
        last_idempotency_key = idempotency_key;
        return snapshot();
      }

      Result<GameSnapshot> offer_draw(const Principal&, const std::string&, std::uint64_t version,
                                      const std::string& idempotency_key) override
      {
        last_expected_version = version;
        last_idempotency_key = idempotency_key;
        return snapshot();
      }

      Result<GameSnapshot> accept_draw(const Principal&, const std::string&, std::uint64_t version,
                                       const std::string& idempotency_key) override
      {
        last_expected_version = version;
        last_idempotency_key = idempotency_key;
        return snapshot();
      }

    private:
      static GameSnapshot snapshot()
      {
        GameSnapshot game;
        game.game_id = kGameId;
        game.version = 2;
        game.fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        game.side_to_move = "white";
        game.status = "ongoing";
        game.legal_moves = {{"e2e4", "e4"}};
        game.move_count = 0;
        game.viewer_seat = "white";
        game.white_player = {"White"};
        game.server_time = "2026-11-06T12:00:00Z";
        return game;
      }
  };

  class HttpApiTest : public ::testing::Test
  {
    protected:
      void SetUp() override
      {
        Chess::Backend::register_http_api(server_, authenticator_, application_);
        port_ = server_.bind_to_any_port("127.0.0.1");
        ASSERT_GT(port_, 0);
        server_thread_ = std::thread([this] { server_.listen_after_bind(); });
        for (int attempt = 0; attempt < 100 && !server_.is_running(); ++attempt)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        ASSERT_TRUE(server_.is_running());
        client_ = std::make_unique<httplib::Client>("127.0.0.1", port_);
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

      httplib::Headers authenticated_headers() const
      {
        return {
          {"Cookie", kSessionCookie},
          {"Origin", kOrigin},
          {"X-CSRF-Token", kCsrfToken},
          {"Idempotency-Key", "0123456789abcdef"}
        };
      }

      TestAuthenticator authenticator_;
      TestGameApplication application_;
      httplib::Server server_;
      int port_ = 0;
      std::thread server_thread_;
      std::unique_ptr<httplib::Client> client_;
  };

  TEST_F(HttpApiTest, LivenessIsPublicAndResponsesAreNotCached)
  {
    const auto response = client_->Get("/api/v1/health/live");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);
    EXPECT_EQ(response->get_header_value("Cache-Control"), "private, no-store");
    EXPECT_EQ(Json::parse(response->body)["status"], "ok");
  }

  TEST_F(HttpApiTest, ReadinessReflectsApplicationDependencies)
  {
    application_.ready = false;
    const auto response = client_->Get("/api/v1/health/ready");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 503);
    EXPECT_EQ(response->get_header_value("Content-Type").find("application/problem+json"), 0u);
  }

  TEST_F(HttpApiTest, GameRoutesRequireAuthentication)
  {
    const auto response = client_->Get(std::string("/api/v1/games/") + kGameId);
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 401);
  }

  TEST_F(HttpApiTest, UnauthenticatedWritesAreRejectedBeforeHeaderValidation)
  {
    const auto response = client_->Post("/api/v1/games", httplib::Headers{}, "{}", "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 401);
    EXPECT_EQ(application_.create_calls, 0);
  }

  TEST_F(HttpApiTest, SessionReturnsCsrfTokenWithoutPrincipalDetails)
  {
    httplib::Headers headers{{"Cookie", kSessionCookie}};
    const auto response = client_->Get("/api/v1/session", headers);
    ASSERT_TRUE(response);
    ASSERT_EQ(response->status, 200);
    const Json body = Json::parse(response->body);
    EXPECT_EQ(body.size(), 1u);
    EXPECT_EQ(body["csrfToken"], kCsrfToken);
    EXPECT_EQ(response->get_header_value("Cache-Control"), "private, no-store");
  }

  TEST_F(HttpApiTest, CreateGameRequiresCsrfAndReturnsVersionedSnapshot)
  {
    auto headers = authenticated_headers();
    const auto response = client_->Post("/api/v1/games", headers, "{}", "application/json");
    ASSERT_TRUE(response);
    ASSERT_EQ(response->status, 201);
    EXPECT_EQ(response->get_header_value("ETag"), "\"2\"");
    EXPECT_EQ(response->get_header_value("Location"), std::string("/api/v1/games/") + kGameId);
    EXPECT_EQ(Json::parse(response->body)["gameId"], kGameId);
    EXPECT_EQ(application_.create_calls, 1);
    EXPECT_EQ(application_.last_idempotency_key, "0123456789abcdef");
  }

  TEST_F(HttpApiTest, ForgedOriginIsRejectedBeforeApplicationCall)
  {
    auto headers = authenticated_headers();
    headers.erase("Origin");
    headers.emplace("Origin", "https://attacker.example");
    const auto response = client_->Post("/api/v1/games", headers, "{}", "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 403);
    EXPECT_EQ(application_.create_calls, 0);
  }

  TEST_F(HttpApiTest, MoveRequiresCurrentStrongEtag)
  {
    auto headers = authenticated_headers();
    const auto response = client_->Post(std::string("/api/v1/games/") + kGameId + "/moves",
                                        headers, R"({"uci":"e2e4"})", "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 428);
    EXPECT_EQ(application_.move_calls, 0);
  }

  TEST_F(HttpApiTest, MoveRequiresValidUciAndDelegatesExpectedVersion)
  {
    auto headers = authenticated_headers();
    headers.emplace("If-Match", "\"7\"");
    const auto response = client_->Post(std::string("/api/v1/games/") + kGameId + "/moves",
                                        headers, R"({"uci":"e2e4"})", "application/json");
    ASSERT_TRUE(response);
    ASSERT_EQ(response->status, 200);
    EXPECT_EQ(application_.move_calls, 1);
    EXPECT_EQ(application_.last_expected_version, 7u);
    EXPECT_EQ(application_.last_idempotency_key, "0123456789abcdef");
  }

  TEST_F(HttpApiTest, JoinAndLifecycleActionRoutesAcceptVersionedRequests)
  {
    auto headers = authenticated_headers();
    headers.emplace("If-Match", "\"7\"");
    const std::string base = std::string("/api/v1/games/") + kGameId;

    const auto joined = client_->Post(base, headers, "", "application/json");
    ASSERT_TRUE(joined);
    EXPECT_EQ(joined->status, 200);

    const auto resigned = client_->Post(base + "/resign", headers, "", "application/json");
    ASSERT_TRUE(resigned);
    EXPECT_EQ(resigned->status, 200);

    const auto offered = client_->Post(base + "/draw-offers", headers, "", "application/json");
    ASSERT_TRUE(offered);
    EXPECT_EQ(offered->status, 200);

    const auto accepted = client_->Post(base + "/draw-offers/accept", headers, "", "application/json");
    ASSERT_TRUE(accepted);
    EXPECT_EQ(accepted->status, 200);
  }

  TEST_F(HttpApiTest, UnknownJsonFieldsAreRejected)
  {
    auto headers = authenticated_headers();
    headers.emplace("If-Match", "\"7\"");
    const auto response = client_->Post(std::string("/api/v1/games/") + kGameId + "/moves",
                                        headers, R"({"uci":"e2e4","color":"black"})", "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_EQ(application_.move_calls, 0);
  }

  TEST_F(HttpApiTest, InvalidMoveShapeIsRejectedWithoutEchoingInput)
  {
    auto headers = authenticated_headers();
    headers.emplace("If-Match", "\"7\"");
    const auto response = client_->Post(std::string("/api/v1/games/") + kGameId + "/moves",
                                        headers, R"({"uci":"secret-invalid-value"})", "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 422);
    EXPECT_EQ(response->body.find("secret-invalid-value"), std::string::npos);
    EXPECT_EQ(application_.move_calls, 0);
  }

  TEST_F(HttpApiTest, LegalMovesAndHistoryAreAuthenticatedAndSerialized)
  {
    const auto legal_moves = client_->Get(std::string("/api/v1/games/") + kGameId + "/legal-moves", authenticated_headers());
    ASSERT_TRUE(legal_moves);
    ASSERT_EQ(legal_moves->status, 200);
    EXPECT_EQ(legal_moves->get_header_value("ETag"), "\"2\"");
    EXPECT_EQ(Json::parse(legal_moves->body)["legalMoves"][0]["uci"], "e2e4");

    const auto history = client_->Get(std::string("/api/v1/games/") + kGameId + "/moves?limit=10", authenticated_headers());
    ASSERT_TRUE(history);
    ASSERT_EQ(history->status, 200);
    EXPECT_EQ(Json::parse(history->body)["moves"][0]["san"], "e4");
  }

  TEST_F(HttpApiTest, HistoryPageSizeIsBounded)
  {
    const auto response = client_->Get(std::string("/api/v1/games/") + kGameId + "/moves?limit=201", authenticated_headers());
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
  }

  TEST_F(HttpApiTest, OversizedRequestBodyIsRejected)
  {
    auto headers = authenticated_headers();
    const std::string body(17 * 1024, 'x');
    const auto response = client_->Post("/api/v1/games", headers, body, "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 413);
    EXPECT_NE(response->body.find("payload_too_large"), std::string::npos);
  }
} // namespace