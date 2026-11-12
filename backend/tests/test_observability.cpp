#include "execution_budget.h"
#include "http_api.h"
#include "service_runtime.h"
#include "sqlite_game_application.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
  using namespace Chess::Backend;
  using Json = nlohmann::json;

  class ObservabilityServer
  {
    public:
      explicit ObservabilityServer(httplib::Server& server) : server_(server)
      {
        server_.new_task_queue = [] { return new httplib::ThreadPool(1); };
        port = server_.bind_to_any_port("127.0.0.1");
        if (port <= 0) throw std::runtime_error("Test bind failed.");
        thread_ = std::thread([this] { server_.listen_after_bind(); });
        server_.wait_until_ready();
      }

      ~ObservabilityServer()
      {
        server_.stop();
        if (thread_.joinable()) thread_.join();
      }

      int port = 0;

    private:
      httplib::Server& server_;
      std::thread thread_;
  };

  class ObservedEvents
  {
    public:
      void append(std::string text)
      {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(Json::parse(text));
        ready_.notify_all();
      }

      std::vector<Json> wait_for(std::size_t count)
      {
        std::unique_lock<std::mutex> lock(mutex_);
        ready_.wait_for(lock, std::chrono::seconds(3), [&] { return events_.size() >= count; });
        return events_;
      }

    private:
      std::mutex mutex_;
      std::condition_variable ready_;
      std::vector<Json> events_;
  };

  bool valid_request_id(const std::string& value)
  {
    return value.size() == 16 && std::all_of(value.begin(), value.end(), [](char digit) {
      return (digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f');
    });
  }

  bool no_current_budget()
  {
    bool clock_used = false;
    ExecutionBudget probe(std::chrono::milliseconds(1), [&] {
      clock_used = true;
      return std::chrono::steady_clock::time_point{};
    });
    return clock_used;
  }

  TEST(RuntimeObservabilityTest, RealHttpLogsOnlySafeFieldsAndCountsUnauthorizedAndPeerLimits)
  {
    SessionAuthenticator auth({"https://chess.test"});
    const auto session = auth.create_session(Principal{"PRIVATE_PRINCIPAL"});
    ASSERT_TRUE(session);
    SqliteDatabase database(":memory:");
    SqliteGameApplication application(database);
    ServiceRateLimiter limiter({4, 4, 3, 10, std::chrono::seconds(60)});
    ObservedEvents events;
    RuntimeObserver observer([&](std::string text) { events.append(std::move(text)); });
    httplib::Server server;
    register_http_api(server, auth, application);
    configure_service_runtime(server, auth, limiter, &observer);
    ObservabilityServer running(server);
    httplib::Client client("127.0.0.1", running.port);
    client.set_read_timeout(3);
    const httplib::Headers headers{{"Cookie", "__Host-ichess_session=" + session->token},
      {"Authorization", "Bearer PRIVATE_AUTHORIZATION"}, {"X-Request-ID", "ATTACKER_REQUEST_ID"},
      {"X-CSRF-Token", "PRIVATE_CSRF"}, {"Idempotency-Key", "PRIVATE_IDEMPOTENCY"},
      {"X-Forwarded-For", "PRIVATE_PEER"}, {"User-Agent", "PRIVATE_USER_AGENT"}};
    const auto malicious = client.Post("/api/v1/games/PRIVATE_PATH%0AFAKE_LOG?secret=PRIVATE_QUERY",
                                       headers, "PRIVATE_BODY", "text/plain");
    ASSERT_TRUE(malicious);
    const auto missing = client.Get("/api/v1/session");
    ASSERT_TRUE(missing);
    EXPECT_EQ(missing->status, 401);
    const auto health = client.Get("/api/v1/health/live", headers);
    ASSERT_TRUE(health);
    EXPECT_EQ(health->status, 200);
    const auto limited = client.Get("/api/v1/session", headers);
    ASSERT_TRUE(limited);
    EXPECT_EQ(limited->status, 429);
    const auto captured = events.wait_for(4);
    ASSERT_EQ(captured.size(), 4U);
    const std::array<const httplib::Response*, 4> responses{
      &*malicious, &*missing, &*health, &*limited};
    for (std::size_t index = 0; index < captured.size(); ++index)
    {
      const auto& event = captured[index];
      EXPECT_EQ(event.size(), 5U);
      EXPECT_TRUE(valid_request_id(event.at("requestId").get<std::string>()));
      EXPECT_EQ(responses[index]->get_header_value_count("X-Request-ID"), 1U);
      EXPECT_EQ(event.at("requestId"), responses[index]->get_header_value("X-Request-ID"));
      EXPECT_EQ(event.at("status"), responses[index]->status);
      EXPECT_GE(event.at("durationMs").get<double>(), 0.0);
      EXPECT_EQ(event.dump().find("PRIVATE_"), std::string::npos);
      EXPECT_EQ(event.dump().find("ATTACKER_"), std::string::npos);
      EXPECT_EQ(event.dump().find(session->token), std::string::npos);
      for (std::size_t previous = 0; previous < index; ++previous)
        EXPECT_NE(event.at("requestId"), captured[previous].at("requestId"));
    }
    EXPECT_EQ(captured[0].at("method"), "POST");
    EXPECT_EQ(captured[0].at("route"), "games");
    EXPECT_EQ(captured[1].at("route"), "session");
    EXPECT_EQ(captured[2].at("route"), "health");
    const auto metrics = observer.snapshot();
    EXPECT_EQ(metrics.total, 4U);
    EXPECT_EQ(metrics.by_status[2], 1U);
    EXPECT_EQ(metrics.by_status[4], 3U);
    EXPECT_GE(metrics.total_duration_ms, 0.0);
  }

  TEST(RuntimeObservabilityTest, PrincipalLimitedResponsesAreObserved)
  {
    SessionAuthenticator auth({"https://chess.test"});
    const auto session = auth.create_session(Principal{"alice"});
    ASSERT_TRUE(session);
    ServiceRateLimiter limiter({4, 4, 10, 1, std::chrono::seconds(60)});
    ObservedEvents events;
    RuntimeObserver observer([&](std::string text) { events.append(std::move(text)); });
    httplib::Server server;
    configure_service_runtime(server, auth, limiter, &observer);
    server.Get("/api/v1/session", [](const httplib::Request&, httplib::Response& response) {
      response.set_content("{}", "application/json");
    });
    ObservabilityServer running(server);
    httplib::Client client("127.0.0.1", running.port);
    client.set_read_timeout(3);
    const httplib::Headers headers{{"Cookie", "__Host-ichess_session=" + session->token}};
    const auto first = client.Get("/api/v1/session", headers);
    ASSERT_TRUE(first);
    EXPECT_EQ(first->status, 200);
    const auto limited = client.Get("/api/v1/session", headers);
    ASSERT_TRUE(limited);
    EXPECT_EQ(limited->status, 429);
    const auto captured = events.wait_for(2);
    ASSERT_EQ(captured.size(), 2U);
    EXPECT_EQ(captured[1].at("status"), 429);
    EXPECT_EQ(captured[1].at("requestId"), limited->get_header_value("X-Request-ID"));
    EXPECT_EQ(observer.snapshot().by_status[4], 1U);
  }

  TEST(RuntimeObservabilityTest, ThrowingSinkCannotBreakServingOrRetainBudget)
  {
    SessionAuthenticator auth({"https://chess.test"});
    ServiceRateLimiter limiter;
    std::atomic<int> cleared{0};
    ObservedEvents events;
    RuntimeObserver observer([&](std::string text) {
      if (no_current_budget()) ++cleared;
      events.append(std::move(text));
      throw std::runtime_error("PRIVATE_SINK_FAILURE");
    });
    httplib::Server server;
    configure_service_runtime(server, auth, limiter, &observer);
    server.Get("/probe", [](const httplib::Request&, httplib::Response& response) {
      response.set_header("X-Request-ID", "HANDLER_OVERRIDE");
      response.set_header("x-request-id", "SECOND_OVERRIDE");
      response.set_content("ok", "text/plain");
    });
    ObservabilityServer running(server);
    httplib::Client client("127.0.0.1", running.port);
    client.set_read_timeout(3);
    client.set_keep_alive(true);
    for (int attempt = 0; attempt < 2; ++attempt)
    {
      const auto result = client.Get("/probe", {{"X-Request-ID", "CLIENT_OVERRIDE"}});
      ASSERT_TRUE(result);
      EXPECT_EQ(result->status, 200);
      EXPECT_EQ(result->body, "ok");
      EXPECT_TRUE(valid_request_id(result->get_header_value("X-Request-ID")));
      EXPECT_EQ(result->get_header_value_count("X-Request-ID"), 1U);
    }
    const auto captured = events.wait_for(2);
    ASSERT_EQ(captured.size(), 2U);
    EXPECT_EQ(cleared.load(), 2);
    EXPECT_EQ(captured[0].at("route"), "other");
    EXPECT_EQ(observer.snapshot().total, 2U);
  }

  TEST(RuntimeObservabilityTest, DeadlineResponseKeepsIdAndNextRequestHasFreshBudget)
  {
    SessionAuthenticator auth({"https://chess.test"});
    ServiceRateLimiter limiter;
    ObservedEvents events;
    std::atomic<int> cleared{0};
    RuntimeObserver observer([&](std::string text) {
      if (no_current_budget()) ++cleared;
      events.append(std::move(text));
    });
    httplib::Server server;
    configure_service_runtime(server, auth, limiter, &observer, {std::chrono::milliseconds(1)});
    server.set_exception_handler([](const httplib::Request&, httplib::Response& response,
                                    std::exception_ptr error) {
      try { std::rethrow_exception(error); }
      catch (const ExecutionDeadlineExceeded&) { response.status = 503; }
      catch (...) { response.status = 500; }
    });
    server.Get("/deadline", [](const httplib::Request&, httplib::Response&) {
      if (no_current_budget()) throw std::runtime_error("Missing request budget.");
      for (;;) ExecutionBudget::checkpoint();
    });
    server.Get("/fresh", [](const httplib::Request&, httplib::Response& response) {
      if (no_current_budget()) throw std::runtime_error("Missing request budget.");
      response.set_content("ok", "text/plain");
    });
    ObservabilityServer running(server);
    httplib::Client client("127.0.0.1", running.port);
    client.set_read_timeout(3);
    client.set_keep_alive(true);
    const auto deadline = client.Get("/deadline");
    ASSERT_TRUE(deadline);
    EXPECT_EQ(deadline->status, 503);
    EXPECT_TRUE(valid_request_id(deadline->get_header_value("X-Request-ID")));
    const auto fresh = client.Get("/fresh");
    ASSERT_TRUE(fresh);
    EXPECT_EQ(fresh->status, 200);
    const auto captured = events.wait_for(2);
    ASSERT_EQ(captured.size(), 2U);
    EXPECT_EQ(captured[0].at("requestId"), deadline->get_header_value("X-Request-ID"));
    EXPECT_NE(captured[0].at("requestId"), captured[1].at("requestId"));
    EXPECT_EQ(cleared.load(), 2);
    EXPECT_EQ(observer.snapshot().by_status[5], 1U);
  }

  TEST(RuntimeObservabilityTest, AggregateStorageIsFixedAndThreadSafeWithoutSink)
  {
    RuntimeObserver observer;
    std::vector<std::thread> threads;
    for (int worker = 0; worker < 4; ++worker)
      threads.emplace_back([&] {
        for (int count = 0; count < 100; ++count)
        {
          observer.record({1, RuntimeMethod::Get, RuntimeRoute::Health, 200, 1.5});
          observer.record({2, RuntimeMethod::Other, RuntimeRoute::Other, 999, 0.5});
        }
      });
    for (auto& thread : threads) thread.join();
    const auto metrics = observer.snapshot();
    EXPECT_EQ(metrics.total, 800U);
    EXPECT_EQ(metrics.by_status.size(), 6U);
    EXPECT_EQ(metrics.by_status[0], 400U);
    EXPECT_EQ(metrics.by_status[2], 400U);
    EXPECT_DOUBLE_EQ(metrics.total_duration_ms, 800.0);
  }

  TEST(RuntimeObservabilityTest, RuntimeRejectsInvalidExecutionTimeout)
  {
    SessionAuthenticator auth({"https://chess.test"});
    ServiceRateLimiter limiter;
    httplib::Server server;
    EXPECT_THROW(configure_service_runtime(server, auth, limiter, nullptr,
                 {std::chrono::milliseconds(0)}), std::runtime_error);
    EXPECT_THROW(configure_service_runtime(server, auth, limiter, nullptr,
                 {std::chrono::milliseconds(60001)}), std::runtime_error);
  }
}