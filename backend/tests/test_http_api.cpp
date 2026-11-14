#include "http_api.h"
#include "sqlite_database.h"
#include "sqlite_game_application.h"

#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
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
      mutable int history_calls = 0;
      mutable std::uint64_t last_history_limit = 0;
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
                                        std::uint64_t, std::uint64_t limit) const override
      {
        ++history_calls;
        last_history_limit = limit;
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
      virtual HttpApiConfig configuration() const { return {}; }
      virtual std::function<bool()> additional_readiness() const { return {}; }

      void SetUp() override
      {
        Chess::Backend::register_http_api(server_, authenticator_, application_, configuration(),
                                          additional_readiness());
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

  class HttpApiAdditionalReadinessTest : public HttpApiTest
  {
    protected:
      std::function<bool()> additional_readiness() const override { return [] { return false; }; }
  };

  TEST_F(HttpApiAdditionalReadinessTest, ReadinessRequiresEveryConfiguredDependency)
  {
    const auto response = client_->Get("/api/v1/health/ready");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 503);
    EXPECT_EQ(Json::parse(response->body)["code"], "not_ready");
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

  TEST_F(HttpApiTest, HistoryPageSizeZeroIsRejected)
  {
    const auto response = client_->Get(std::string("/api/v1/games/") + kGameId + "/moves?limit=0", authenticated_headers());
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_NE(response->body.find("invalid_query"), std::string::npos);
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

  TEST_F(HttpApiTest, DuplicateJsonFieldsAreRejectedBeforeApplicationCall)
  {
    auto headers = authenticated_headers();
    headers.emplace("If-Match", "\"7\"");
    const auto response = client_->Post(std::string("/api/v1/games/") + kGameId + "/moves",
                                        headers, R"({"uci":"a1a2","\u0075ci":"e2e4"})", "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_EQ(Json::parse(response->body)["code"], "invalid_json");
    EXPECT_EQ(application_.move_calls, 0);
  }

  TEST_F(HttpApiTest, DuplicateNestedJsonFieldsAreRejectedBeforeApplicationCall)
  {
    const auto response = client_->Post("/api/v1/games", authenticated_headers(),
                                        R"({"timeControl":{"initialMs":1000,"initialMs":2000}})", "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_EQ(Json::parse(response->body)["code"], "invalid_json");
    EXPECT_EQ(application_.create_calls, 0);
  }

  TEST_F(HttpApiTest, ExcessiveJsonNestingIsRejectedAndServerRemainsAvailable)
  {
    const std::string body = "{\"timeControl\":" + std::string(2000, '[') + "0" + std::string(2000, ']') + "}";
    const auto response = client_->Post("/api/v1/games", authenticated_headers(), body, "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_EQ(Json::parse(response->body)["code"], "invalid_json");
    EXPECT_EQ(application_.create_calls, 0);

    const auto health = client_->Get("/api/v1/health/live");
    ASSERT_TRUE(health);
    EXPECT_EQ(health->status, 200);
  }

  TEST_F(HttpApiTest, JsonNumericOverflowIsRejectedWithoutEchoingInput)
  {
    const auto response = client_->Post("/api/v1/games", authenticated_headers(),
                                        R"({"timeControl":{"initialMs":1e10000}})", "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 400);
    EXPECT_EQ(Json::parse(response->body)["code"], "invalid_json");
    EXPECT_EQ(response->body.find("1e10000"), std::string::npos);
    EXPECT_EQ(application_.create_calls, 0);
  }

  TEST_F(HttpApiTest, ValidNestedTimeControlIsAccepted)
  {
    const auto response = client_->Post("/api/v1/games", authenticated_headers(),
                                        R"({"timeControl":{"initialMs":60000,"incrementMs":0}})", "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 201);
    EXPECT_EQ(application_.create_calls, 1);
  }

  TEST_F(HttpApiTest, IncrementTimeControlIsRejectedUntilSupported)
  {
    const auto response = client_->Post("/api/v1/games", authenticated_headers(),
        R"({"timeControl":{"initialMs":60000,"incrementMs":1000}})", "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 422);
    EXPECT_EQ(Json::parse(response->body).at("code"), "invalid_time_control");
    EXPECT_EQ(application_.create_calls, 0);
  }

  class HttpApiHistoryLimitTest : public HttpApiTest, public ::testing::WithParamInterface<std::size_t>
  {
    protected:
      HttpApiConfig configuration() const override
      {
        HttpApiConfig config;
        config.max_history_page_size = GetParam();
        return config;
      }
  };

  TEST_P(HttpApiHistoryLimitTest, DefaultPageSizeRespectsConfiguredBound)
  {
    const auto response = client_->Get(std::string("/api/v1/games/") + kGameId + "/moves", authenticated_headers());
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 200);
    const std::size_t expected_limit = GetParam() == 0 ? 1 : (GetParam() < 100 ? GetParam() : 100);
    EXPECT_EQ(application_.last_history_limit, expected_limit);
    EXPECT_EQ(application_.history_calls, 1);
  }

  TEST_P(HttpApiHistoryLimitTest, ExplicitPageSizeCannotExceedEffectiveBound)
  {
    const std::size_t maximum = GetParam() == 0 ? 1 : (GetParam() > 200 ? 200 : GetParam());
    const std::string base = std::string("/api/v1/games/") + kGameId + "/moves?limit=";
    const auto accepted = client_->Get(base + std::to_string(maximum), authenticated_headers());
    ASSERT_TRUE(accepted);
    EXPECT_EQ(accepted->status, 200);
    EXPECT_EQ(application_.last_history_limit, maximum);

    const auto rejected = client_->Get(base + std::to_string(maximum + 1), authenticated_headers());
    ASSERT_TRUE(rejected);
    EXPECT_EQ(rejected->status, 400);
    EXPECT_EQ(application_.history_calls, 1);
  }

  INSTANTIATE_TEST_SUITE_P(ConfiguredBounds, HttpApiHistoryLimitTest,
                          ::testing::Values(std::size_t{0}, std::size_t{1}, std::size_t{50}, std::size_t{200}, std::size_t{500}));

  TEST(SqliteDatabaseSmokeTest, InitializesSchemaAndReopensDurableDatabase)
  {
    const auto path = std::filesystem::temp_directory_path()
                      / ("ichess-backend-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db");
    {
      SqliteDatabase database(path.string());
      ASSERT_TRUE(database.is_ready());
      ASSERT_NE(database.handle(), nullptr);
    }
    {
      SqliteDatabase database(path.string());
      EXPECT_TRUE(database.is_ready());
    }
    std::error_code error;
    std::filesystem::remove(path, error);
    std::filesystem::remove(path.string() + "-wal", error);
    std::filesystem::remove(path.string() + "-shm", error);
  }

  TEST(SqliteDatabaseSmokeTest, InvalidDatabasePathFailsClosed)
  {
    const auto path = std::filesystem::temp_directory_path() / "ichess-missing-directory" / "games.db";
    SqliteDatabase database(path.string());
    EXPECT_FALSE(database.is_ready());
  }

  TEST(SqliteGameApplicationTest, CreatesDurableUntimedGameAndReplaysSameIdempotentResult)
  {
    const auto path = std::filesystem::temp_directory_path()
                      / ("ichess-game-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db");
    const Principal owner{"owner"};
    std::string game_id;
    {
      SqliteDatabase database(path.string());
      ASSERT_TRUE(database.is_ready());
      SqliteGameApplication application(database);
      ASSERT_TRUE(application.is_ready());
      const auto created = application.create_game(owner, CreateGameCommand{}, "0123456789abcdef");
      ASSERT_TRUE(std::holds_alternative<GameSnapshot>(created));
      const GameSnapshot snapshot = std::get<GameSnapshot>(created);
      game_id = snapshot.game_id;
      EXPECT_EQ(snapshot.version, 1u);
      EXPECT_EQ(snapshot.viewer_seat, "white");
      EXPECT_EQ(snapshot.status, "ongoing");
      EXPECT_EQ(snapshot.legal_moves.size(), 20u);

      const auto retry = application.create_game(owner, CreateGameCommand{}, "0123456789abcdef");
      ASSERT_TRUE(std::holds_alternative<GameSnapshot>(retry));
      EXPECT_EQ(std::get<GameSnapshot>(retry).game_id, game_id);
    }
    {
      SqliteDatabase database(path.string());
      SqliteGameApplication application(database);
      const auto loaded = application.get_game(owner, game_id);
      ASSERT_TRUE(std::holds_alternative<GameSnapshot>(loaded));
      EXPECT_EQ(std::get<GameSnapshot>(loaded).fen,
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
      const auto replay = application.create_game(owner, CreateGameCommand{}, "0123456789abcdef");
      ASSERT_TRUE(std::holds_alternative<GameSnapshot>(replay));
      EXPECT_EQ(std::get<GameSnapshot>(replay).game_id, game_id);
    }
    std::error_code cleanup_error;
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove(path.string() + "-wal", cleanup_error);
    std::filesystem::remove(path.string() + "-shm", cleanup_error);
  }

  TEST(SqliteGameApplicationTest, HidesGamesFromNonParticipantsAndRejectsChangedIdempotentPayload)
  {
    const auto path = std::filesystem::temp_directory_path()
                      / ("ichess-game-auth-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db");
    {
      SqliteDatabase database(path.string());
      ASSERT_TRUE(database.is_ready());
      SqliteGameApplication application(database);
      const auto created = application.create_game(Principal{"owner"}, CreateGameCommand{}, "0123456789abcdef");
      ASSERT_TRUE(std::holds_alternative<GameSnapshot>(created));
      const std::string game_id = std::get<GameSnapshot>(created).game_id;

      const auto hidden = application.get_game(Principal{"other"}, game_id);
      ASSERT_TRUE(std::holds_alternative<ApplicationError>(hidden));
      EXPECT_EQ(std::get<ApplicationError>(hidden).status, 404);

      CreateGameCommand different;
      different.initial_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1";
      const auto conflict = application.create_game(Principal{"owner"}, different, "0123456789abcdef");
      ASSERT_TRUE(std::holds_alternative<ApplicationError>(conflict));
      EXPECT_EQ(std::get<ApplicationError>(conflict).status, 409);
    }
    std::error_code cleanup_error;
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove(path.string() + "-wal", cleanup_error);
    std::filesystem::remove(path.string() + "-shm", cleanup_error);
  }

  TEST(SqliteGameApplicationTest, SupportsSimpleTimedGamesAndRejectsIncrementClocks)
  {
    const auto path = std::filesystem::temp_directory_path()
                      / ("ichess-game-input-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db");
    {
      SqliteDatabase database(path.string());
      ASSERT_TRUE(database.is_ready());
      SqliteGameApplication application(database);

      CreateGameCommand invalid;
      invalid.initial_fen = "not a fen";
      const auto invalid_result = application.create_game(Principal{"owner"}, invalid, "0123456789abcdef");
      ASSERT_TRUE(std::holds_alternative<ApplicationError>(invalid_result));
      EXPECT_EQ(std::get<ApplicationError>(invalid_result).status, 422);

      CreateGameCommand timed;
      timed.time_control = TimeControl{60000, 0};
      const auto timed_result = application.create_game(Principal{"owner"}, timed, "fedcba9876543210");
      ASSERT_TRUE(std::holds_alternative<GameSnapshot>(timed_result));
      ASSERT_TRUE(std::get<GameSnapshot>(timed_result).clock);
      EXPECT_EQ(std::get<GameSnapshot>(timed_result).clock->white_remaining_ms, 60000U);
      EXPECT_FALSE(std::get<GameSnapshot>(timed_result).clock->running_color);

      CreateGameCommand incremented;
      incremented.time_control = TimeControl{60000, 1000};
      const auto incremented_result = application.create_game(Principal{"owner"}, incremented, "fedcba9876543211");
      ASSERT_TRUE(std::holds_alternative<ApplicationError>(incremented_result));
      EXPECT_EQ(std::get<ApplicationError>(incremented_result).status, 422);
    }
    std::error_code cleanup_error;
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove(path.string() + "-wal", cleanup_error);
    std::filesystem::remove(path.string() + "-shm", cleanup_error);
  }

  class SqliteApplicationTest : public ::testing::Test
  {
    protected:
      void SetUp() override
      {
        path_ = std::filesystem::temp_directory_path()
                / ("ichess-store-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db");
      }

      void TearDown() override
      {
        std::error_code cleanup_error;
        std::filesystem::remove(path_, cleanup_error);
        std::filesystem::remove(path_.string() + "-wal", cleanup_error);
        std::filesystem::remove(path_.string() + "-shm", cleanup_error);
      }

      std::filesystem::path path_;
  };

  TEST_F(SqliteApplicationTest, ConcurrentCreateRetriesAcrossConnectionsCreateExactlyOneGame)
  {
    SqliteDatabase first_database(path_.string());
    SqliteDatabase second_database(path_.string());
    ASSERT_TRUE(first_database.is_ready());
    ASSERT_TRUE(second_database.is_ready());
    SqliteGameApplication first(first_database);
    SqliteGameApplication second(second_database);
    Result<GameSnapshot> first_result;
    Result<GameSnapshot> second_result;
    std::thread first_thread([&] {
      first_result = first.create_game(Principal{"owner"}, {}, "0123456789abcdef");
    });
    std::thread second_thread([&] {
      second_result = second.create_game(Principal{"owner"}, {}, "0123456789abcdef");
    });
    first_thread.join();
    second_thread.join();
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(first_result));
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(second_result));
    EXPECT_EQ(std::get<GameSnapshot>(first_result).game_id, std::get<GameSnapshot>(second_result).game_id);

    sqlite3_stmt* statement = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(first_database.handle(), "SELECT count(*) FROM games", -1, &statement, nullptr), SQLITE_OK);
    ASSERT_EQ(sqlite3_step(statement), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(statement, 0), 1);
    sqlite3_finalize(statement);
  }

  TEST_F(SqliteApplicationTest, ReplayedCreateRechecksCurrentOwnership)
  {
    SqliteDatabase database(path_.string());
    SqliteGameApplication application(database);
    const auto created = application.create_game(Principal{"owner"}, {}, "0123456789abcdef");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(created));
    ASSERT_EQ(sqlite3_exec(database.handle(), "UPDATE games SET white_principal = 'replacement'", nullptr, nullptr, nullptr), SQLITE_OK);
    const auto replay = application.create_game(Principal{"owner"}, {}, "0123456789abcdef");
    ASSERT_TRUE(std::holds_alternative<ApplicationError>(replay));
    EXPECT_EQ(std::get<ApplicationError>(replay).status, 404);
  }

  TEST_F(SqliteApplicationTest, StorageFailuresAreNotReportedAsMissingGames)
  {
    SqliteDatabase database(path_.string());
    SqliteGameApplication application(database);
    ASSERT_EQ(sqlite3_exec(database.handle(), "DROP TABLE games", nullptr, nullptr, nullptr), SQLITE_OK);
    const auto result = application.get_game(Principal{"owner"}, "abcdefghijklmnopqrstuv");
    ASSERT_TRUE(std::holds_alternative<ApplicationError>(result));
    EXPECT_EQ(std::get<ApplicationError>(result).status, 503);
  }

  TEST_F(SqliteApplicationTest, CustomFenCountersDoNotInventMoveHistory)
  {
    SqliteDatabase database(path_.string());
    SqliteGameApplication application(database);
    CreateGameCommand command;
    command.initial_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 42";
    const auto created = application.create_game(Principal{"owner"}, command, "0123456789abcdef");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(created));
    const auto& snapshot = std::get<GameSnapshot>(created);
    EXPECT_EQ(snapshot.move_count, 0u);
    EXPECT_EQ(snapshot.side_to_move, "black");
    EXPECT_EQ(snapshot.game_id.size(), 32u);
  }

  TEST_F(SqliteApplicationTest, UnavailableDatabaseRejectsApplicationOperations)
  {
    SqliteDatabase database((path_ / "missing" / "games.db").string());
    SqliteGameApplication application(database);
    EXPECT_FALSE(application.is_ready());
    const auto created = application.create_game(Principal{"owner"}, {}, "0123456789abcdef");
    ASSERT_TRUE(std::holds_alternative<ApplicationError>(created));
    EXPECT_EQ(std::get<ApplicationError>(created).status, 503);
    const auto loaded = application.get_game(Principal{"owner"}, "abcdefghijklmnopqrstuv");
    ASSERT_TRUE(std::holds_alternative<ApplicationError>(loaded));
    EXPECT_EQ(std::get<ApplicationError>(loaded).status, 503);
  }

  TEST_F(SqliteApplicationTest, FutureSchemaVersionFailsClosedWithoutCreatingTables)
  {
    {
      SqliteDatabase database(path_.string());
      ASSERT_TRUE(database.is_ready());
      ASSERT_EQ(sqlite3_exec(database.handle(), "UPDATE schema_version SET version = 3; DROP TABLE game_moves;",
                            nullptr, nullptr, nullptr), SQLITE_OK);
    }
    SqliteDatabase reopened(path_.string());
    EXPECT_FALSE(reopened.is_ready());
    sqlite3_stmt* statement = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(reopened.handle(),
                                "SELECT count(*) FROM sqlite_schema WHERE name = 'game_moves'", -1, &statement, nullptr), SQLITE_OK);
    ASSERT_EQ(sqlite3_step(statement), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(statement, 0), 0);
    sqlite3_finalize(statement);
  }

  TEST_F(SqliteApplicationTest, RejectsStructurallyInvalidChessPositions)
  {
    SqliteDatabase database(path_.string());
    SqliteGameApplication application(database);
    for (const char* fen : {
           "8/8/8/8/8/8/8/8 w - - 0 1",
           "8/8/8/8/8/8/4k3/4K3 w - - 0 1",
           "4k3/8/8/8/8/8/8/P3K3 w - - 0 1",
           "4k3/8/8/8/8/8/8/4K3 w - - -1 1"})
    {
      CreateGameCommand command;
      command.initial_fen = fen;
      const auto result = application.create_game(Principal{"owner"}, command, "0123456789abcdef");
      ASSERT_TRUE(std::holds_alternative<ApplicationError>(result));
      EXPECT_EQ(std::get<ApplicationError>(result).code, "invalid_fen");
    }
  }

  TEST_F(SqliteApplicationTest, InitialTerminalPositionDoesNotAdvertisePlayableMoves)
  {
    SqliteDatabase database(path_.string());
    SqliteGameApplication application(database);
    CreateGameCommand command;
    command.initial_fen = "4k3/8/8/8/8/8/8/4K3 w - - 0 1";
    const auto result = application.create_game(Principal{"owner"}, command, "0123456789abcdef");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(result));
    const auto& snapshot = std::get<GameSnapshot>(result);
    EXPECT_EQ(snapshot.status, "draw_insufficient_material");
    EXPECT_TRUE(snapshot.legal_moves.empty());
  }

  TEST_F(SqliteApplicationTest, ExpiredIdempotencyKeyCreatesNewGame)
  {
    SqliteDatabase database(path_.string());
    SqliteGameApplication application(database);
    const auto first = application.create_game(Principal{"owner"}, {}, "0123456789abcdef");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(first));
    ASSERT_EQ(sqlite3_exec(database.handle(), "UPDATE idempotency_records SET created_at = '2000-01-01T00:00:00Z'",
                          nullptr, nullptr, nullptr), SQLITE_OK);
    const auto second = application.create_game(Principal{"owner"}, {}, "0123456789abcdef");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(second));
    EXPECT_NE(std::get<GameSnapshot>(first).game_id, std::get<GameSnapshot>(second).game_id);
  }

  TEST_F(SqliteApplicationTest, SameKeyForDifferentPrincipalsCreatesSeparateGames)
  {
    SqliteDatabase database(path_.string());
    SqliteGameApplication application(database);
    const auto first = application.create_game(Principal{"first-owner"}, {}, "0123456789abcdef");
    const auto second = application.create_game(Principal{"second-owner"}, {}, "0123456789abcdef");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(first));
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(second));
    EXPECT_NE(std::get<GameSnapshot>(first).game_id, std::get<GameSnapshot>(second).game_id);
  }

  TEST_F(SqliteApplicationTest, FailedIdempotencyWriteRollsBackGameCreation)
  {
    SqliteDatabase database(path_.string());
    SqliteGameApplication application(database);
    ASSERT_EQ(sqlite3_set_authorizer(database.handle(),
      [](void*, int action, const char* table, const char*, const char*, const char*) {
        return action == SQLITE_INSERT && table != nullptr && std::string_view(table) == "idempotency_records"
                 ? SQLITE_DENY : SQLITE_OK;
      }, nullptr), SQLITE_OK);
    const auto failed = application.create_game(Principal{"owner"}, {}, "0123456789abcdef");
    ASSERT_TRUE(std::holds_alternative<ApplicationError>(failed));
    EXPECT_EQ(std::get<ApplicationError>(failed).status, 500);

    sqlite3_stmt* statement = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(database.handle(), "SELECT count(*) FROM games", -1, &statement, nullptr), SQLITE_OK);
    ASSERT_EQ(sqlite3_step(statement), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(statement, 0), 0);
    sqlite3_finalize(statement);

    ASSERT_EQ(sqlite3_set_authorizer(database.handle(), nullptr, nullptr), SQLITE_OK);
    const auto retry = application.create_game(Principal{"owner"}, {}, "0123456789abcdef");
    EXPECT_TRUE(std::holds_alternative<GameSnapshot>(retry));
  }

  TEST(LocalAuthApiTest, RegistrationLoginAndLogoutUseSecureServerSessions)
  {
    const auto account_path = std::filesystem::temp_directory_path()
        / ("ichess-auth-api-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
    {
      SqliteLocalAccountStore accounts(account_path.string());
      ASSERT_TRUE(accounts.is_ready());
      SessionAuthenticator authenticator({"https://chess.example.test"});
      SqliteDatabase game_database(":memory:");
      SqliteGameApplication application(game_database);
      httplib::Server server;
      register_http_api(server, authenticator, application, {}, {}, &accounts, &authenticator);
      const int port = server.bind_to_any_port("127.0.0.1");
      ASSERT_GT(port, 0);
      std::thread server_thread([&] { server.listen_after_bind(); });
      server.wait_until_ready();
      httplib::Client client("127.0.0.1", port);
      client.set_connection_timeout(2);
      client.set_read_timeout(4);
      const std::string body = R"({"username":"Alice.Player","password":"a sufficiently long password"})";
      const httplib::Headers origin{{"Origin", "https://chess.example.test"}};

      const auto missing_origin = client.Post("/api/v1/auth/register", body, "application/json");
      ASSERT_TRUE(missing_origin);
      EXPECT_EQ(missing_origin->status, 403);
      const auto foreign_origin = client.Post("/api/v1/auth/login", {{"Origin", "https://evil.test"}}, body,
                       "application/json");
      ASSERT_TRUE(foreign_origin);
      EXPECT_EQ(foreign_origin->status, 403);

      const auto registered = client.Post("/api/v1/auth/register", origin, body, "application/json");
      ASSERT_TRUE(registered);
      ASSERT_EQ(registered->status, 201);
      EXPECT_NE(registered->get_header_value("Set-Cookie").find("__Host-ichess_session="), std::string::npos);
      EXPECT_NE(registered->get_header_value("Set-Cookie").find("; Secure; HttpOnly; SameSite=Strict"), std::string::npos);
      EXPECT_EQ(Json::parse(registered->body).at("csrfToken").get<std::string>().size(), 64U);

      const auto duplicate = client.Post("/api/v1/auth/register", origin, body, "application/json");
      ASSERT_TRUE(duplicate);
      EXPECT_EQ(duplicate->status, 409);

        const auto invalid = client.Post("/api/v1/auth/login", origin,
          R"({"username":"alice.player","password":"a different long password"})", "application/json");
      ASSERT_TRUE(invalid);
      EXPECT_EQ(invalid->status, 401);
      EXPECT_EQ(Json::parse(invalid->body).at("detail"), "Username or password is incorrect.");

        const auto logged_in = client.Post("/api/v1/auth/login", origin,
          R"({"username":"ALICE.PLAYER","password":"a sufficiently long password"})", "application/json");
      ASSERT_TRUE(logged_in);
      ASSERT_EQ(logged_in->status, 200);
      const auto login_cookie = logged_in->get_header_value("Set-Cookie");
      const auto semicolon = login_cookie.find(';');
      ASSERT_NE(semicolon, std::string::npos);
      const auto csrf = Json::parse(logged_in->body).at("csrfToken").get<std::string>();
      httplib::Headers logout_headers{{"Cookie", login_cookie.substr(0, semicolon)},
          {"Origin", "https://chess.example.test"}, {"X-CSRF-Token", csrf}};
      const auto logged_out = client.Post("/api/v1/auth/logout", logout_headers, "", "");
      ASSERT_TRUE(logged_out);
      EXPECT_EQ(logged_out->status, 200);
      EXPECT_NE(logged_out->get_header_value("Set-Cookie").find("Max-Age=0"), std::string::npos);
      const auto old_session = client.Get("/api/v1/session", logout_headers);
      ASSERT_TRUE(old_session);
      EXPECT_EQ(old_session->status, 401);

      server.stop();
      server_thread.join();
    }
    std::error_code cleanup_error;
    std::filesystem::remove(account_path, cleanup_error);
    std::filesystem::remove(account_path.string() + "-wal", cleanup_error);
    std::filesystem::remove(account_path.string() + "-shm", cleanup_error);
  }

  TEST(LocalAuthApiTest, RegistrationCanBeClosedWhileLoginRemainsAvailable)
  {
    const auto account_path = std::filesystem::temp_directory_path()
        / ("ichess-auth-closed-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
    {
      SqliteLocalAccountStore accounts(account_path.string());
      SessionAuthenticator authenticator({"https://chess.example.test"});
      SqliteDatabase game_database(":memory:");
      SqliteGameApplication application(game_database);
      HttpApiConfig config;
      config.allow_registration = false;
      httplib::Server server;
      register_http_api(server, authenticator, application, config, {}, &accounts, &authenticator);
      const int port = server.bind_to_any_port("127.0.0.1");
      ASSERT_GT(port, 0);
      std::thread server_thread([&] { server.listen_after_bind(); });
      server.wait_until_ready();
      httplib::Client client("127.0.0.1", port);
      client.set_read_timeout(4);
        const httplib::Headers origin{{"Origin", "https://chess.example.test"}};
        const auto registration = client.Post("/api/v1/auth/register", origin,
          R"({"username":"alice","password":"a sufficiently long password"})", "application/json");
      ASSERT_TRUE(registration);
      EXPECT_EQ(registration->status, 404);
        const auto login = client.Post("/api/v1/auth/login", origin,
          R"({"username":"alice","password":"a sufficiently long password"})", "application/json");
      ASSERT_TRUE(login);
      EXPECT_EQ(login->status, 401);
      server.stop();
      server_thread.join();
    }
    std::error_code cleanup_error;
    std::filesystem::remove(account_path, cleanup_error);
    std::filesystem::remove(account_path.string() + "-wal", cleanup_error);
    std::filesystem::remove(account_path.string() + "-shm", cleanup_error);
  }

    TEST(LocalAuthApiTest, PasswordChangeRevokesPriorSessionsAndIssuesFreshSession)
    {
    const auto account_path = std::filesystem::temp_directory_path()
      / ("ichess-auth-password-change-"
        + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
    {
      SqliteLocalAccountStore accounts(account_path.string());
      SessionAuthenticator authenticator({"https://chess.example.test"});
      SqliteDatabase game_database(":memory:");
      SqliteGameApplication application(game_database);
      httplib::Server server;
      register_http_api(server, authenticator, application, {}, {}, &accounts, &authenticator);
      const int port = server.bind_to_any_port("127.0.0.1");
      ASSERT_GT(port, 0);
      std::thread server_thread([&] { server.listen_after_bind(); });
      server.wait_until_ready();
      httplib::Client client("127.0.0.1", port);
      client.set_connection_timeout(2);
      client.set_read_timeout(10);
      const httplib::Headers origin{{"Origin", "https://chess.example.test"}};
      const auto registered = client.Post("/api/v1/auth/register", origin,
        R"({"username":"change-user","password":"original secure password"})", "application/json");
      ASSERT_TRUE(registered);
      ASSERT_EQ(registered->status, 201);
      const auto registration_cookie = registered->get_header_value("Set-Cookie").substr(
        0, registered->get_header_value("Set-Cookie").find(';'));

      const auto logged_in = client.Post("/api/v1/auth/login", origin,
        R"({"username":"change-user","password":"original secure password"})", "application/json");
      ASSERT_TRUE(logged_in);
      ASSERT_EQ(logged_in->status, 200);
      const auto old_cookie = logged_in->get_header_value("Set-Cookie").substr(
        0, logged_in->get_header_value("Set-Cookie").find(';'));
      const auto csrf = Json::parse(logged_in->body).at("csrfToken").get<std::string>();
      httplib::Headers change_headers{{"Cookie", old_cookie}, {"Origin", "https://chess.example.test"},
                      {"X-CSRF-Token", csrf}};
      const auto changed = client.Post("/api/v1/auth/password", change_headers,
        R"({"currentPassword":"original secure password","newPassword":"replacement secure password"})",
        "application/json");
      ASSERT_TRUE(changed);
      ASSERT_EQ(changed->status, 200);
      const auto new_cookie = changed->get_header_value("Set-Cookie").substr(
        0, changed->get_header_value("Set-Cookie").find(';'));
      const auto new_csrf = Json::parse(changed->body).at("csrfToken").get<std::string>();
      EXPECT_EQ(client.Get("/api/v1/session", {{"Cookie", old_cookie}})->status, 401);
      EXPECT_EQ(client.Get("/api/v1/session", {{"Cookie", registration_cookie}})->status, 401);
      EXPECT_EQ(client.Get("/api/v1/session", {{"Cookie", new_cookie}})->status, 200);
      EXPECT_EQ(client.Post("/api/v1/auth/login", origin,
        R"({"username":"change-user","password":"original secure password"})", "application/json")->status, 401);
      const auto new_login = client.Post("/api/v1/auth/login", origin,
        R"({"username":"change-user","password":"replacement secure password"})", "application/json");
      ASSERT_TRUE(new_login);
      EXPECT_EQ(new_login->status, 200);
      EXPECT_EQ(new_csrf.size(), 64U);

      server.stop();
      server_thread.join();
    }
    std::error_code cleanup_error;
    std::filesystem::remove(account_path, cleanup_error);
    std::filesystem::remove(account_path.string() + "-wal", cleanup_error);
    std::filesystem::remove(account_path.string() + "-shm", cleanup_error);
    }

  TEST(LocalAuthApiTest, AccountDeletionAnonymizesGamesAndRevokesSessions)
  {
    const auto account_path = std::filesystem::temp_directory_path()
        / ("ichess-auth-delete-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
    {
      SqliteLocalAccountStore accounts(account_path.string());
      SessionAuthenticator authenticator({"https://chess.example.test"});
      SqliteDatabase game_database(":memory:");
      SqliteGameApplication application(game_database);
      ASSERT_EQ(accounts.create_account("delete-player", "account deletion password"), AccountCreateResult::Created);
      ASSERT_EQ(accounts.create_account("keep-player", "another deletion password"), AccountCreateResult::Created);
      const auto created = application.create_game(Principal{"delete-player"}, {}, "delete-game-key-00001");
      ASSERT_TRUE(std::holds_alternative<GameSnapshot>(created));
      const auto game = std::get<GameSnapshot>(created);
      const auto joined = application.join_game(Principal{"keep-player"}, game.game_id, game.version,
                                                  "delete-join-key-000001");
      ASSERT_TRUE(std::holds_alternative<GameSnapshot>(joined));
      auto deletion_session = authenticator.create_session(Principal{"delete-player"});
      ASSERT_TRUE(deletion_session);
      auto retained_session = authenticator.create_session(Principal{"keep-player"});
      ASSERT_TRUE(retained_session);
      httplib::Server server;
      register_http_api(server, authenticator, application, {}, {}, &accounts, &authenticator);
      const int port = server.bind_to_any_port("127.0.0.1");
      ASSERT_GT(port, 0);
      std::thread server_thread([&] { server.listen_after_bind(); });
      server.wait_until_ready();
      httplib::Client client("127.0.0.1", port);
      client.set_read_timeout(10);
      const httplib::Headers deletion_headers{{"Cookie", "__Host-ichess_session=" + deletion_session->token},
          {"Origin", "https://chess.example.test"}, {"X-CSRF-Token", deletion_session->csrf_token}};
      const auto deleted = client.Post("/api/v1/auth/delete", deletion_headers,
          R"({"currentPassword":"account deletion password"})", "application/json");
      ASSERT_TRUE(deleted);
      EXPECT_EQ(deleted->status, 200);
      EXPECT_EQ(Json::parse(deleted->body).at("status"), "deleted");
      EXPECT_NE(deleted->get_header_value("Set-Cookie").find("Max-Age=0"), std::string::npos);
      EXPECT_FALSE(accounts.authenticate("delete-player", "account deletion password"));
      EXPECT_FALSE(authenticator.create_session(Principal{"delete-player"}));
      httplib::Request old_session_request;
      old_session_request.set_header("Cookie", "__Host-ichess_session=" + deletion_session->token);
      EXPECT_FALSE(authenticator.principal_for(old_session_request));

      const auto retained = application.get_game(Principal{"keep-player"}, game.game_id);
      ASSERT_TRUE(std::holds_alternative<GameSnapshot>(retained));
      EXPECT_EQ(std::get<GameSnapshot>(retained).move_count, 0U);
      sqlite3_stmt* statement = nullptr;
      ASSERT_EQ(sqlite3_prepare_v2(game_database.handle(),
          "SELECT white_principal FROM games WHERE game_id=?1", -1, &statement, nullptr), SQLITE_OK);
      ASSERT_EQ(sqlite3_bind_text(statement, 1, game.game_id.c_str(), -1, SQLITE_TRANSIENT), SQLITE_OK);
      ASSERT_EQ(sqlite3_step(statement), SQLITE_ROW);
      const auto* anonymized = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
      ASSERT_NE(anonymized, nullptr);
      EXPECT_NE(std::string(anonymized).find("deleted-"), std::string::npos);
      sqlite3_finalize(statement);

      server.stop();
      server_thread.join();
    }
    std::error_code cleanup_error;
    std::filesystem::remove(account_path, cleanup_error);
    std::filesystem::remove(account_path.string() + "-wal", cleanup_error);
    std::filesystem::remove(account_path.string() + "-shm", cleanup_error);
  }
} // namespace
