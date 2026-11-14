#include "service_runtime.h"
#include "http_api.h"
#include "sqlite_game_application.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <array>
#include <condition_variable>
#include <fstream>
#include <future>
#include <limits>
#include <mutex>
#ifdef _WIN32
#include <windows.h>
#include <aclapi.h>
#else
#include <sys/stat.h>
#endif

namespace
{
  using namespace Chess::Backend;
  using Json = nlohmann::json;

  std::string credentials(std::int64_t expires = 1700000100)
  {
    return Json::array({Json{{"principal", "alice"}, {"token", std::string(64, 'a')},
                             {"expires", expires}}}).dump();
  }

  TEST(PreviewConfigurationTest, StrictBoundedJsonRejectsAmbiguityAndInvalidExpiry)
  {
    constexpr std::int64_t now = 1700000000;
    ASSERT_EQ(parse_preview_sessions(credentials(), now).size(), 1U);
    EXPECT_EQ(parse_preview_sessions(credentials(), now)[0].principal.id, "alice");
    for (const auto& text : {std::string{}, std::string("[]"), std::string("{}"), std::string("[null]"),
        std::string(65537, ' '), credentials(now), credentials(now + 86401),
        std::string("[{\"principal\":\"alice\",\"principal\":\"bob\",\"token\":\"")
            + std::string(64, 'a') + "\",\"expires\":1700000100}]",
        std::string("[[[[[[[]]]]]]]"), credentials() + "trailing"})
      EXPECT_THROW(parse_preview_sessions(text, now), std::runtime_error);
    auto parsed = Json::parse(credentials());
    for (const auto& expiry : {Json(nullptr), Json("1700000100"), Json(1700000100.0),
                               Json(-1), Json(18446744073709551615ULL)})
    {
      parsed[0]["expires"] = expiry;
      EXPECT_THROW(parse_preview_sessions(parsed.dump(), now), std::runtime_error);
    }
    parsed = Json::parse(credentials());
    parsed[0]["token"] = "short";
    EXPECT_THROW(parse_preview_sessions(parsed.dump(), now), std::runtime_error);
    parsed = Json::parse(credentials());
    parsed[0]["principal"] = "bad\nprincipal";
    EXPECT_THROW(parse_preview_sessions(parsed.dump(), now), std::runtime_error);
    parsed = Json::parse(credentials());
    parsed[0]["extra"] = true;
    EXPECT_THROW(parse_preview_sessions(parsed.dump(), now), std::runtime_error);
    parsed = Json::parse(credentials());
    parsed.push_back(parsed[0]);
    EXPECT_THROW(parse_preview_sessions(parsed.dump(), now), std::runtime_error);
    parsed[1]["token"] = std::string(64, 'b');
    EXPECT_THROW(parse_preview_sessions(parsed.dump(), now, 1), std::runtime_error);
    EXPECT_EQ(parse_preview_sessions(parsed.dump(), now).size(), 2U);
  }

  TEST(PreviewConfigurationTest, ErrorsNeverIncludeCredentialText)
  {
    try
    {
      parse_preview_sessions("SECRET_INVALID_CONTENT", 1700000000);
      FAIL();
    }
    catch (const std::exception& error)
    {
      EXPECT_EQ(std::string(error.what()).find("SECRET"), std::string::npos);
    }
  }

  TEST(PreviewConfigurationTest, PortsAndLocalDurablePathsAreRequired)
  {
    EXPECT_EQ(parse_service_port("1"), 1);
    EXPECT_EQ(parse_service_port("65535"), 65535);
    for (const auto* port : {"", "0", "65536", "-1", "+80", " 80", "80x", "99999999999"})
      EXPECT_THROW(parse_service_port(port), std::runtime_error);
    const auto root = std::filesystem::current_path();
    ServiceRuntimeConfig config{"https://chess.test", root / "private-preview.json", root / "preview.sqlite",
                  root / "preview-revocations.sqlite", root / "preview-accounts.sqlite", 8080};
    EXPECT_NO_THROW(validate_runtime_config(config));
    config.session_file.clear();
    EXPECT_NO_THROW(validate_runtime_config(config));
    config.session_file = root / "private-preview.json";
    config.database_file = ":memory:";
    EXPECT_THROW(validate_runtime_config(config), std::runtime_error);
    config.database_file = "relative.sqlite";
    EXPECT_THROW(validate_runtime_config(config), std::runtime_error);
    config.database_file = root;
    EXPECT_THROW(validate_runtime_config(config), std::runtime_error);
    config.database_file = config.session_file;
    EXPECT_THROW(validate_runtime_config(config), std::runtime_error);
    config.database_file = root / "preview.sqlite";
    config.revocation_database_file = config.database_file;
    EXPECT_THROW(validate_runtime_config(config), std::runtime_error);
    config.revocation_database_file = config.session_file;
    EXPECT_THROW(validate_runtime_config(config), std::runtime_error);
    config.revocation_database_file = root / "separate" / "revocations.sqlite";
    EXPECT_THROW(validate_runtime_config(config), std::runtime_error);
    config.revocation_database_file = root / "preview-revocations.sqlite";
    config.account_database_file = config.revocation_database_file;
    EXPECT_THROW(validate_runtime_config(config), std::runtime_error);
    config.account_database_file = root / "preview-accounts.sqlite";
    config.port = 0;
    EXPECT_THROW(validate_runtime_config(config), std::runtime_error);
    config.port = 8080;
    config.public_origin = "http://chess.test";
    EXPECT_THROW(validate_runtime_config(config), std::runtime_error);
  }

  TEST(StorageReserveTest, ParsesUnsignedByteThresholdAndRejectsMalformedValues)
  {
    EXPECT_EQ(parse_storage_reserve("0"), 0U);
    EXPECT_EQ(parse_storage_reserve("67108864"), 67108864U);
    for (const auto* value : {"", "-1", "+1", "1x", "18446744073709551616"})
      EXPECT_THROW(parse_storage_reserve(value), std::runtime_error);
  }

  TEST(StorageReserveTest, ChecksAvailableSpaceAndFailsClosedForMissingPaths)
  {
    const auto root = std::filesystem::temp_directory_path();
    EXPECT_TRUE(storage_reserve_available(root, 1));
    EXPECT_TRUE(storage_reserve_available(root, 0));
    EXPECT_FALSE(storage_reserve_available(root, (std::numeric_limits<std::uint64_t>::max)()));
    EXPECT_FALSE(storage_reserve_available(root / "missing-storage-path", 0));
  }

  TEST(ServiceRateLimiterTest, CapsBothKeySpacesAndResetsAtTtlWithoutEvictionByAttackers)
  {
    auto now = std::chrono::steady_clock::time_point{};
    ServiceRateLimiter limiter({2, 1, 2, 1, std::chrono::seconds(10)}, [&] { return now; });
    EXPECT_TRUE(limiter.allow_peer("127.0.0.1"));
    EXPECT_TRUE(limiter.allow_peer("127.0.0.1"));
    EXPECT_FALSE(limiter.allow_peer("127.0.0.1"));
    EXPECT_TRUE(limiter.allow_peer("127.0.0.2"));
    for (int index = 0; index < 100; ++index)
      EXPECT_FALSE(limiter.allow_peer("peer-" + std::to_string(index)));
    EXPECT_EQ(limiter.state_size().first, 2U);
    EXPECT_TRUE(limiter.allow_principal(Principal{"alice"}));
    EXPECT_FALSE(limiter.allow_principal(Principal{"alice"}));
    EXPECT_FALSE(limiter.allow_principal(Principal{"bob"}));
    EXPECT_EQ(limiter.state_size().second, 1U);
    EXPECT_FALSE(limiter.allow_peer(std::string(129, 'x')));
    EXPECT_FALSE(limiter.allow_peer(""));
    now += std::chrono::seconds(9);
    EXPECT_FALSE(limiter.allow_peer("127.0.0.1"));
    now += std::chrono::seconds(1);
    EXPECT_TRUE(limiter.allow_peer("new-peer"));
    EXPECT_TRUE(limiter.allow_principal(Principal{"bob"}));
    EXPECT_EQ(limiter.state_size().first, 1U);
    EXPECT_EQ(limiter.state_size().second, 1U);
  }

  TEST(ServiceRateLimiterTest, ConcurrentRequestsCannotExceedQuota)
  {
    ServiceRateLimiter limiter({2, 2, 50, 50, std::chrono::seconds(60)});
    std::atomic<int> allowed{0};
    std::vector<std::thread> threads;
    for (int index = 0; index < 4; ++index)
      threads.emplace_back([&] {
        for (int attempt = 0; attempt < 50; ++attempt)
          if (limiter.allow_principal(Principal{"alice"})) ++allowed;
      });
    for (auto& thread : threads) thread.join();
    EXPECT_EQ(allowed.load(), 50);
    EXPECT_EQ(limiter.state_size().second, 1U);
  }

  TEST(ServiceRateLimiterTest, OperationBudgetsAreIndependentBoundedAndExpire)
  {
    auto now = std::chrono::steady_clock::time_point{};
    RateLimitConfig config;
    config.max_principals = 1;
    config.create_requests = 1;
    config.join_requests = 1;
    config.mutation_requests = 1;
    ServiceRateLimiter limiter(config, [&] { return now; });
    const Principal alice{"alice"};
    EXPECT_TRUE(limiter.allow_operation(alice, "POST", "/api/v1/games"));
    EXPECT_FALSE(limiter.allow_operation(alice, "POST", "/api/v1/games"));
    EXPECT_TRUE(limiter.allow_operation(alice, "POST", "/api/v1/games/first"));
    EXPECT_FALSE(limiter.allow_operation(alice, "POST", "/api/v1/games/second"));
    EXPECT_TRUE(limiter.allow_operation(alice, "POST", "/api/v1/games/first/moves"));
    EXPECT_FALSE(limiter.allow_operation(alice, "POST", "/api/v1/games/second/resign"));
    EXPECT_TRUE(limiter.allow_operation(alice, "GET", "/api/v1/games/first"));
    EXPECT_FALSE(limiter.allow_operation(Principal{"bob"}, "POST", "/api/v1/games"));
    now += config.window;
    EXPECT_TRUE(limiter.allow_operation(Principal{"bob"}, "POST", "/api/v1/games"));
  }

  TEST(ServiceRateLimiterTest, AnonymousLoginAndRegistrationBudgetsAreIndependent)
  {
    auto now = std::chrono::steady_clock::time_point{};
    RateLimitConfig config;
    config.login_requests = 2;
    config.registration_requests = 1;
    ServiceRateLimiter limiter(config, [&] { return now; });
    EXPECT_TRUE(limiter.allow_auth_attempt("127.0.0.1", false));
    EXPECT_TRUE(limiter.allow_auth_attempt("127.0.0.1", false));
    EXPECT_FALSE(limiter.allow_auth_attempt("127.0.0.1", false));
    EXPECT_TRUE(limiter.allow_auth_attempt("127.0.0.1", true));
    EXPECT_FALSE(limiter.allow_auth_attempt("127.0.0.1", true));
    EXPECT_TRUE(limiter.allow_auth_attempt("127.0.0.2", true));
    now += config.window;
    EXPECT_TRUE(limiter.allow_auth_attempt("127.0.0.1", true));
  }

  class RunningTestServer
  {
    public:
      explicit RunningTestServer(httplib::Server& server) : server_(server)
      {
        port = server_.bind_to_any_port("127.0.0.1");
        if (port <= 0) throw std::runtime_error("Test bind failed.");
        thread_ = std::thread([this] { server_.listen_after_bind(); });
        server_.wait_until_ready();
      }
      ~RunningTestServer()
      {
        server_.stop();
        if (thread_.joinable()) thread_.join();
      }
      int port = 0;
    private:
      httplib::Server& server_;
      std::thread thread_;
  };

  TEST(ServiceRuntimeTest, ActualApiBootstrapsOnceAndEnforcesOriginAndRateLimit)
  {
    SessionAuthenticator auth({"https://chess.test"});
    const auto session = auth.create_session(Principal{"alice"});
    ASSERT_TRUE(session);
    SqliteDatabase database(":memory:");
    SqliteGameApplication application(database);
    ASSERT_TRUE(application.is_ready());
    ServiceRateLimiter limiter({4, 4, 100, 4, std::chrono::seconds(60)});
    httplib::Server server;
    register_http_api(server, auth, application);
    configure_service_runtime(server, auth, limiter);
    RunningTestServer running(server);
    httplib::Client client("127.0.0.1", running.port);
    client.set_read_timeout(3);
    httplib::Headers headers{{"Cookie", "__Host-ichess_session=" + session->token}};
    const auto bootstrap = client.Get("/api/v1/session", headers);
    ASSERT_TRUE(bootstrap);
    ASSERT_EQ(bootstrap->status, 200);
    EXPECT_EQ(Json::parse(bootstrap->body)["csrfToken"], session->csrf_token);
    const auto second = client.Get("/api/v1/session", headers);
    ASSERT_TRUE(second);
    EXPECT_EQ(second->status, 200);
    headers.emplace("X-CSRF-Token", session->csrf_token);
    headers.emplace("Idempotency-Key", "preview-create-1");
    headers.emplace("Origin", "https://evil.test");
    const auto forbidden = client.Post("/api/v1/games", headers, "{}", "application/json");
    ASSERT_TRUE(forbidden);
    EXPECT_EQ(forbidden->status, 403);
    headers.erase("Origin");
    headers.emplace("Origin", "https://chess.test");
    const auto created = client.Post("/api/v1/games", headers, "{}", "application/json");
    ASSERT_TRUE(created);
    EXPECT_EQ(created->status, 201);
    const auto limited = client.Get("/api/v1/session", headers);
    ASSERT_TRUE(limited);
    EXPECT_EQ(limited->status, 429);
    EXPECT_EQ(limited->get_header_value("Retry-After"), "60");
    EXPECT_EQ(limited->get_header_value("Cache-Control"), "no-store");
    EXPECT_EQ(limited->get_header_value("Content-Type"), "application/problem+json");
    EXPECT_EQ(Json::parse(limited->body)["status"], 429);
    EXPECT_EQ(limiter.state_size().first, 1U);
    EXPECT_EQ(limiter.state_size().second, 1U);
  }

  TEST(PasswordWorkGateTest, BoundsConcurrentWorkAndReleasesCapacity)
  {
    PasswordWorkGate gate(2);
    EXPECT_TRUE(gate.try_acquire());
    EXPECT_TRUE(gate.try_acquire());
    EXPECT_FALSE(gate.try_acquire());
    gate.release();
    EXPECT_TRUE(gate.try_acquire());
    gate.release();
    gate.release();
    EXPECT_TRUE(gate.try_acquire());
    gate.release();
    EXPECT_THROW(PasswordWorkGate invalid(0), std::runtime_error);
    EXPECT_THROW(PasswordWorkGate invalid(5), std::runtime_error);
  }

  TEST(ServiceRuntimeTest, UnauthenticatedHealthIsPeerLimitedAndForwardedHeadersCannotBypass)
  {
    SessionAuthenticator auth({"https://chess.test"});
    ServiceRateLimiter limiter({4, 4, 1, 10, std::chrono::seconds(60)});
    httplib::Server server;
    configure_service_runtime(server, auth, limiter);
    server.Get("/api/v1/health/live", [](const httplib::Request&, httplib::Response& response) {
      response.set_content("{}", "application/json");
    });
    RunningTestServer running(server);
    httplib::Client client("127.0.0.1", running.port);
    client.set_read_timeout(3);
    ASSERT_EQ(client.Get("/api/v1/health/live")->status, 200);
    const auto rejected = client.Get("/api/v1/health/live", {{"CF-Connecting-IP", "192.0.2.1"},
                                                           {"X-Forwarded-For", "192.0.2.2"}});
    ASSERT_TRUE(rejected);
    EXPECT_EQ(rejected->status, 429);
    EXPECT_EQ(limiter.state_size().first, 1U);
    EXPECT_EQ(limiter.state_size().second, 0U);
  }

  TEST(ServiceRuntimeTest, AuthenticatedGameplayRetriesRevocationAndReadinessUseRealAdapters)
  {
    SessionAuthenticator auth({"https://chess.test"});
    const auto white = auth.create_session(Principal{"white-player"});
    const auto black = auth.create_session(Principal{"black-player"});
    ASSERT_TRUE(white);
    ASSERT_TRUE(black);
    SqliteDatabase database(":memory:");
    SqliteGameApplication application(database);
    ServiceRateLimiter limiter;
    httplib::Server server;
    register_http_api(server, auth, application);
    configure_service_runtime(server, auth, limiter);
    RunningTestServer running(server);
    httplib::Client client("127.0.0.1", running.port);
    client.set_read_timeout(3);
    const auto headers_for = [](const SessionCredentials& session, const std::string& key) {
      return httplib::Headers{{"Cookie", "__Host-ichess_session=" + session.token},
                              {"Origin", "https://chess.test"}, {"X-CSRF-Token", session.csrf_token},
                              {"Idempotency-Key", key}};
    };
    const auto created = client.Post("/api/v1/games", headers_for(*white, "create-game-key-0001"), "{}", "application/json");
    ASSERT_TRUE(created);
    ASSERT_EQ(created->status, 201);
    const auto base = "/api/v1/games/" + Json::parse(created->body).at("gameId").get<std::string>();
    auto join_headers = headers_for(*black, "join-game-key-000001");
    join_headers.emplace("If-Match", created->get_header_value("ETag"));
    const auto joined = client.Post(base, join_headers, "{}", "application/json");
    ASSERT_TRUE(joined);
    ASSERT_EQ(joined->status, 200);
    EXPECT_EQ(Json::parse(joined->body).at("viewerSeat"), "black");
    auto move_headers = headers_for(*white, "submit-move-key-0001");
    move_headers.emplace("If-Match", joined->get_header_value("ETag"));
    const auto moved = client.Post(base + "/moves", move_headers, R"({"uci":"e2e4"})", "application/json");
    ASSERT_TRUE(moved);
    ASSERT_EQ(moved->status, 200);
    EXPECT_EQ(Json::parse(moved->body).at("version"), 3);
    const auto retry = client.Post(base + "/moves", move_headers, R"({"uci":"e2e4"})", "application/json");
    ASSERT_TRUE(retry);
    EXPECT_EQ(retry->status, 200);
    EXPECT_EQ(retry->body, moved->body);
    const auto history = client.Get(base + "/moves", headers_for(*black, "read-history-key-001"));
    ASSERT_TRUE(history);
    ASSERT_EQ(history->status, 200);
    EXPECT_EQ(Json::parse(history->body).at("moves").size(), 1U);
    EXPECT_TRUE(auth.revoke_session(white->token));
    const auto revoked = client.Get(base, headers_for(*white, "read-game-key-00001"));
    ASSERT_TRUE(revoked);
    EXPECT_EQ(revoked->status, 401);
    {
      std::lock_guard<std::mutex> lock(database.mutex());
      ASSERT_EQ(sqlite3_exec(database.handle(), "DROP TABLE game_moves", nullptr, nullptr, nullptr), SQLITE_OK);
    }
    const auto readiness = client.Get("/api/v1/health/ready");
    ASSERT_TRUE(readiness);
    EXPECT_EQ(readiness->status, 503);
  }

  TEST(ServiceRuntimeTest, ShutdownMonitorStopsListenerAndJoinsWithoutSignal)
  {
    httplib::Server server;
    volatile std::sig_atomic_t signal = 0;
    {
      ShutdownMonitor monitor(server, signal);
    }
    signal = 1;
    ASSERT_GT(server.bind_to_any_port("127.0.0.1"), 0);
    ShutdownMonitor monitor(server, signal);
    auto listener = std::async(std::launch::async, [&] { return server.listen_after_bind(); });
    EXPECT_EQ(listener.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    if (server.is_running()) server.stop();
    EXPECT_TRUE(listener.get());
  }

  TEST(ServiceRuntimeTest, MissingCredentialFileFailsClosed)
  {
    SessionAuthenticator auth({"https://chess.test"});
    EXPECT_THROW(load_preview_sessions(std::filesystem::temp_directory_path()
        / "ichess-nonexistent-private-preview-session-file.json", auth), std::runtime_error);
    EXPECT_EQ(auth.session_count(), 0U);
  }

  TEST(ServiceRuntimeTest, ConfiguredThreadPoolRejectsBeyondSixtyFourQueuedJobs)
  {
    SessionAuthenticator auth({"https://chess.test"});
    ServiceRateLimiter limiter;
    httplib::Server server;
    configure_service_runtime(server, auth, limiter);
    std::unique_ptr<httplib::TaskQueue> pool(server.new_task_queue());
    std::promise<void> release;
    const auto gate = release.get_future().share();
    std::promise<void> workers_ready;
    auto ready = workers_ready.get_future();
    std::atomic<int> started{0};
    for (int index = 0; index < 4; ++index)
      EXPECT_TRUE(pool->enqueue([&] {
        if (++started == 4) workers_ready.set_value();
        gate.wait();
      }));
    EXPECT_EQ(ready.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    for (int index = 0; index < 64; ++index) EXPECT_TRUE(pool->enqueue([] {}));
    EXPECT_FALSE(pool->enqueue([] {}));
    release.set_value();
    pool->shutdown();
  }

  class TemporarySessionFile
  {
    public:
      explicit TemporarySessionFile(const std::string& content)
      {
        SessionAuthenticator random({"https://chess.test"});
        const auto session = random.create_session(Principal{"test"});
        if (!session) throw std::runtime_error("Test initialization failed.");
        path = std::filesystem::temp_directory_path() / ("ichess-session-test-" + session->token + ".json");
        std::ofstream output(path, std::ios::binary);
        if (!output.write(content.data(), static_cast<std::streamsize>(content.size())))
          throw std::runtime_error("Test file initialization failed.");
        output.close();
      }
      ~TemporarySessionFile()
      {
        std::error_code error;
        std::filesystem::remove(path, error);
      }
      bool restrict_access(bool owner_only)
      {
#ifdef _WIN32
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
        alignas(TOKEN_USER) std::array<unsigned char, sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE> storage{};
        DWORD required = 0;
        const auto obtained = GetTokenInformation(token, TokenUser, storage.data(),
                                                 static_cast<DWORD>(storage.size()), &required);
        CloseHandle(token);
        if (!obtained) return false;
        const auto* user = reinterpret_cast<const TOKEN_USER*>(storage.data());
        alignas(DWORD) std::array<unsigned char, SECURITY_MAX_SID_SIZE> world{};
        DWORD world_size = static_cast<DWORD>(world.size());
        if (!CreateWellKnownSid(WinWorldSid, nullptr, world.data(), &world_size)) return false;
        EXPLICIT_ACCESSW access{};
        access.grfAccessPermissions = GENERIC_ALL;
        access.grfAccessMode = SET_ACCESS;
        access.grfInheritance = NO_INHERITANCE;
        access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        access.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
        access.Trustee.ptstrName = static_cast<LPWSTR>(owner_only ? user->User.Sid : world.data());
        PACL acl = nullptr;
        if (SetEntriesInAclW(1, &access, nullptr, &acl) != ERROR_SUCCESS) return false;
        const auto result = SetNamedSecurityInfoW(const_cast<LPWSTR>(path.c_str()), SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
            user->User.Sid, nullptr, acl, nullptr);
        LocalFree(acl);
        return result == ERROR_SUCCESS;
#else
        return chmod(path.c_str(), owner_only ? 0600 : 0644) == 0;
#endif
      }
      std::filesystem::path path;
  };

  TEST(ServiceRuntimeTest, ImportsProtectedFileAndRejectsSharedOrOversizedFile)
  {
    const auto expires = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + 300;
    TemporarySessionFile file(credentials(expires));
    ASSERT_TRUE(file.restrict_access(true));
    SessionAuthenticator auth({"https://chess.test"});
    EXPECT_NO_THROW(load_preview_sessions(file.path, auth));
    EXPECT_EQ(auth.session_count(), 1U);
    httplib::Request request;
    request.set_header("Cookie", "__Host-ichess_session=" + std::string(64, 'a'));
    EXPECT_EQ(auth.authenticate(request, false).principal.id, "alice");
    ASSERT_TRUE(file.restrict_access(false));
    SessionAuthenticator other({"https://chess.test"});
    EXPECT_THROW(load_preview_sessions(file.path, other), std::runtime_error);
    EXPECT_EQ(other.session_count(), 0U);
    TemporarySessionFile oversized(std::string(65537, 'x'));
    ASSERT_TRUE(oversized.restrict_access(true));
    EXPECT_THROW(load_preview_sessions(oversized.path, other), std::runtime_error);
    EXPECT_EQ(other.session_count(), 0U);
  }
}