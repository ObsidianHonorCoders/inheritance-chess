#include "http_api.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <string_view>
#include <system_error>
#include <utility>

namespace Chess::Backend
{
  namespace
  {
    using Json = nlohmann::json;

    void set_security_headers(httplib::Response& response)
    {
      response.set_header("Cache-Control", "private, no-store");
      response.set_header("X-Content-Type-Options", "nosniff");
      response.set_header("Referrer-Policy", "no-referrer");
    }

    void write_json(httplib::Response& response, int status, const Json& value)
    {
      set_security_headers(response);
      response.status = status;
      response.set_content(value.dump(), "application/json; charset=utf-8");
    }

    void write_problem(httplib::Response& response,
                       int status,
                       std::string_view code,
                       std::string_view title,
                       std::string_view detail)
    {
      set_security_headers(response);
      response.status = status;
      response.set_content(Json{
        {"type", "about:blank"},
        {"title", title},
        {"status", status},
        {"detail", detail},
        {"code", code}
      }.dump(), "application/problem+json; charset=utf-8");
    }

    void write_problem(httplib::Response& response, const ApplicationError& error)
    {
      const int status = (error.status >= 400 && error.status <= 599) ? error.status : 500;
      const std::string code = error.code.empty() ? "internal_error" : error.code;
      const std::string title = error.title.empty() ? "Request Failed" : error.title;
      const std::string detail = error.detail.empty() ? "The request could not be completed." : error.detail;
      Json body{
        {"type", "about:blank"},
        {"title", title},
        {"status", status},
        {"detail", detail},
        {"code", code}
      };
      if (error.current_version.has_value())
      {
        body["currentVersion"] = *error.current_version;
      }
      set_security_headers(response);
      response.status = status;
      response.set_content(body.dump(), "application/problem+json; charset=utf-8");
    }

    bool authenticate(const httplib::Request& request,
                      const IRequestAuthenticator& authenticator,
                      bool state_changing,
                      httplib::Response& response,
                      Principal& principal,
                      std::string* csrf_token = nullptr)
    {
      const AuthenticationResult result = authenticator.authenticate(request, state_changing);
      if (!result.authenticated || result.principal.id.empty())
      {
        const bool forbidden = result.failure_status == 403;
        write_problem(response,
                      forbidden ? 403 : 401,
                      forbidden ? "request_forbidden" : "unauthenticated",
                      forbidden ? "Forbidden" : "Unauthorized",
                      forbidden ? "The request could not be verified." : "Authentication is required.");
        return false;
      }

      if (state_changing)
      {
        const std::string token = request.get_header_value("X-CSRF-Token");
        if (token.size() < 32 || token.size() > 256)
        {
          write_problem(response, 403, "csrf_rejected", "Forbidden", "The request could not be verified.");
          return false;
        }
      }

      principal = result.principal;
      if (csrf_token != nullptr)
      {
        *csrf_token = result.csrf_token;
      }
      return true;
    }

    bool require_header(const httplib::Request& request,
                        const char* header_name,
                        std::size_t minimum_length,
                        std::size_t maximum_length,
                        httplib::Response& response)
    {
      const std::string value = request.get_header_value(header_name);
      if (value.size() < minimum_length || value.size() > maximum_length)
      {
        write_problem(response, 400, "invalid_header", "Bad Request", "A required request header is missing or invalid.");
        return false;
      }
      return true;
    }

    bool parse_expected_version(const httplib::Request& request,
                                httplib::Response& response,
                                std::uint64_t& version)
    {
      if (!request.has_header("If-Match"))
      {
        write_problem(response, 428, "precondition_required", "Precondition Required", "If-Match is required.");
        return false;
      }

      const std::string etag = request.get_header_value("If-Match");
      if (etag.size() < 3 || etag.front() != '"' || etag.back() != '"')
      {
        write_problem(response, 400, "invalid_if_match", "Bad Request", "If-Match must contain a strong game version ETag.");
        return false;
      }

      const std::string_view digits(etag.data() + 1, etag.size() - 2);
      const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), version);
      if (parsed.ec != std::errc{} || parsed.ptr != digits.data() + digits.size() || version == 0)
      {
        write_problem(response, 400, "invalid_if_match", "Bad Request", "If-Match must contain a strong game version ETag.");
        return false;
      }
      return true;
    }

    bool parse_unsigned_query(const httplib::Request& request,
                              const char* name,
                              std::uint64_t fallback,
                              std::uint64_t maximum,
                              httplib::Response& response,
                              std::uint64_t& value)
    {
      value = fallback;
      if (!request.has_param(name))
      {
        return true;
      }

      const std::string raw = request.get_param_value(name);
      const auto parsed = std::from_chars(raw.data(), raw.data() + raw.size(), value);
      if (raw.empty() || parsed.ec != std::errc{} || parsed.ptr != raw.data() + raw.size() || value > maximum)
      {
        write_problem(response, 400, "invalid_query", "Bad Request", "A query parameter is invalid.");
        return false;
      }
      return true;
    }

    bool is_json_request(const httplib::Request& request)
    {
      std::string content_type = request.get_header_value("Content-Type");
      const auto separator = content_type.find(';');
      if (separator != std::string::npos)
      {
        content_type.resize(separator);
      }
      for (char& character : content_type)
      {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
      }
      return content_type == "application/json";
    }

    bool parse_json_body(const httplib::Request& request,
                         httplib::Response& response,
                         Json& body,
                         bool body_optional = false)
    {
      if (request.body.empty() && body_optional)
      {
        body = Json::object();
        return true;
      }
      if (request.body.empty() || !is_json_request(request))
      {
        write_problem(response, 400, "invalid_request", "Bad Request", "A JSON request body is required.");
        return false;
      }
      try
      {
        body = Json::parse(request.body);
      }
      catch (const Json::parse_error&)
      {
        write_problem(response, 400, "invalid_json", "Bad Request", "The request body is not valid JSON.");
        return false;
      }
      if (!body.is_object())
      {
        write_problem(response, 400, "invalid_json_shape", "Bad Request", "The JSON request body must be an object.");
        return false;
      }
      return true;
    }

    bool has_only_fields(const Json& body, std::initializer_list<std::string_view> allowed_fields)
    {
      for (auto field = body.begin(); field != body.end(); ++field)
      {
        bool allowed = false;
        for (const std::string_view name : allowed_fields)
        {
          if (field.key() == name)
          {
            allowed = true;
            break;
          }
        }
        if (!allowed)
        {
          return false;
        }
      }
      return true;
    }

    Json to_json(const LegalMove& move)
    {
      return Json{{"uci", move.uci}, {"san", move.san}};
    }

    Json to_json(const GameSnapshot& game)
    {
      Json black_player = nullptr;
      if (game.black_player.has_value())
      {
        black_player = Json{{"displayName", game.black_player->display_name}};
      }

      Json clock = nullptr;
      if (game.clock.has_value())
      {
        clock = Json{
          {"whiteRemainingMs", game.clock->white_remaining_ms},
          {"blackRemainingMs", game.clock->black_remaining_ms},
          {"runningColor", game.clock->running_color.has_value() ? Json(*game.clock->running_color) : Json(nullptr)},
          {"incrementMs", game.clock->increment_ms}
        };
      }

      Json legal_moves = Json::array();
      for (const auto& move : game.legal_moves)
      {
        legal_moves.push_back(to_json(move));
      }

      return Json{
        {"gameId", game.game_id},
        {"version", game.version},
        {"fen", game.fen},
        {"sideToMove", game.side_to_move},
        {"status", game.status},
        {"legalMoves", std::move(legal_moves)},
        {"moveCount", game.move_count},
        {"viewerSeat", game.viewer_seat},
        {"players", {
          {"white", {{"displayName", game.white_player.display_name}}},
          {"black", std::move(black_player)}
        }},
        {"clock", std::move(clock)},
        {"serverTime", game.server_time}
      };
    }

    Json to_json(const MovePage& page)
    {
      Json moves = Json::array();
      for (const auto& move : page.moves)
      {
        moves.push_back(Json{{"ply", move.ply}, {"uci", move.uci}, {"san", move.san}});
      }
      return Json{
        {"moves", std::move(moves)},
        {"nextAfterPly", page.next_after_ply.has_value() ? Json(*page.next_after_ply) : Json(nullptr)}
      };
    }

    bool valid_game_id(std::string_view game_id);

    void write_snapshot_result(httplib::Response& response, Result<GameSnapshot> result, int success_status,
                               bool set_location = false)
    {
      if (std::holds_alternative<ApplicationError>(result))
      {
        write_problem(response, std::get<ApplicationError>(result));
        return;
      }
      const auto& game = std::get<GameSnapshot>(result);
      if (!valid_game_id(game.game_id) || game.version == 0)
      {
        write_problem(response, 500, "invalid_application_response", "Internal Server Error",
                      "The request could not be completed.");
        return;
      }
      if (set_location)
      {
        response.set_header("Location", "/api/v1/games/" + game.game_id);
      }
      response.set_header("ETag", "\"" + std::to_string(game.version) + "\"");
      write_json(response, success_status, to_json(game));
    }

    bool authenticate_request(const httplib::Request& request,
                              const IRequestAuthenticator& authenticator,
                              bool write,
                              httplib::Response& response,
                              Principal& principal)
    {
      if (!authenticate(request, authenticator, write, response, principal))
      {
        return false;
      }
      return !write || require_header(request, "Idempotency-Key", 16, 128, response);
    }

    bool valid_game_id(std::string_view game_id)
    {
      if (game_id.size() < 22 || game_id.size() > 64)
      {
        return false;
      }
      for (const char character : game_id)
      {
        const bool allowed = (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')
                             || (character >= '0' && character <= '9') || character == '_' || character == '-';
        if (!allowed)
        {
          return false;
        }
      }
      return true;
    }

    std::string path_id(const httplib::Request& request)
    {
      return request.matches.size() > 1 ? request.matches[1].str() : std::string{};
    }
  } // namespace

  void register_http_api(httplib::Server& server,
                         const IRequestAuthenticator& authenticator,
                         IGameApplication& application,
                         const HttpApiConfig& config)
  {
    server.set_payload_max_length(std::clamp(config.max_request_body_bytes, std::size_t{1}, std::size_t{64 * 1024}));
    server.set_read_timeout(std::clamp(config.read_timeout_seconds, std::time_t{1}, std::time_t{60}));
    server.set_write_timeout(std::clamp(config.write_timeout_seconds, std::time_t{1}, std::time_t{60}));
    server.set_error_handler([](const httplib::Request&, httplib::Response& response) {
      if (!response.body.empty())
      {
        return;
      }
      if (response.status == 413)
      {
        write_problem(response, 413, "payload_too_large", "Content Too Large", "The request body exceeds the allowed size.");
        return;
      }
      if (response.status == 405)
      {
        write_problem(response, 405, "method_not_allowed", "Method Not Allowed", "The request method is not supported.");
        return;
      }
      if (response.status == 404)
      {
        write_problem(response, 404, "not_found", "Not Found", "The requested resource was not found.");
      }
    });
    server.set_exception_handler([](const httplib::Request&, httplib::Response& response, std::exception_ptr) {
      write_problem(response, 500, "internal_error", "Internal Server Error", "The request could not be completed.");
    });

    server.Get("/api/v1/health/live", [](const httplib::Request&, httplib::Response& response) {
      write_json(response, 200, Json{{"status", "ok"}});
    });
    server.Get("/api/v1/health/ready", [&application](const httplib::Request&, httplib::Response& response) {
      if (!application.is_ready())
      {
        write_problem(response, 503, "not_ready", "Service Unavailable", "A required service dependency is unavailable.");
        return;
      }
      write_json(response, 200, Json{{"status", "ok"}});
    });
    server.Get("/api/v1/session", [&authenticator](const httplib::Request& request, httplib::Response& response) {
      Principal principal;
      std::string csrf_token;
      if (!authenticate(request, authenticator, false, response, principal, &csrf_token))
      {
        return;
      }
      if (csrf_token.size() < 32 || csrf_token.size() > 256)
      {
        write_problem(response, 503, "session_not_ready", "Service Unavailable", "The session could not be initialized.");
        return;
      }
      write_json(response, 200, Json{{"csrfToken", csrf_token}});
    });
    server.Post("/api/v1/games", [&authenticator, &application](const httplib::Request& request, httplib::Response& response) {
      Principal principal;
      if (!authenticate_request(request, authenticator, true, response, principal))
      {
        return;
      }

      Json body;
      if (!parse_json_body(request, response, body, true))
      {
        return;
      }
      if (!has_only_fields(body, {"initialFen", "timeControl"}))
      {
        write_problem(response, 400, "unknown_field", "Bad Request", "The request contains an unsupported field.");
        return;
      }

      CreateGameCommand command;
      if (body.contains("initialFen"))
      {
        if (!body["initialFen"].is_string())
        {
          write_problem(response, 400, "invalid_fen", "Bad Request", "initialFen must be a string.");
          return;
        }
        const std::string fen = body["initialFen"].get<std::string>();
        if (fen.empty() || fen.size() > 128)
        {
          write_problem(response, 422, "invalid_fen", "Unprocessable Content", "The initial position is invalid.");
          return;
        }
        command.initial_fen = fen;
      }
      if (body.contains("timeControl"))
      {
        const Json& time_control = body["timeControl"];
        if (!time_control.is_object() || !has_only_fields(time_control, {"initialMs", "incrementMs"})
            || !time_control.contains("initialMs") || !time_control["initialMs"].is_number_unsigned())
        {
          write_problem(response, 400, "invalid_time_control", "Bad Request", "The time control is invalid.");
          return;
        }
        TimeControl parsed;
        parsed.initial_ms = time_control["initialMs"].get<std::uint64_t>();
        if (parsed.initial_ms > 86400000)
        {
          write_problem(response, 422, "invalid_time_control", "Unprocessable Content", "The time control is out of range.");
          return;
        }
        if (time_control.contains("incrementMs"))
        {
          if (!time_control["incrementMs"].is_number_unsigned())
          {
            write_problem(response, 400, "invalid_time_control", "Bad Request", "The time control is invalid.");
            return;
          }
          parsed.increment_ms = time_control["incrementMs"].get<std::uint64_t>();
          if (parsed.increment_ms > 3600000)
          {
            write_problem(response, 422, "invalid_time_control", "Unprocessable Content", "The time control is out of range.");
            return;
          }
        }
        command.time_control = parsed;
      }

      const std::string key = request.get_header_value("Idempotency-Key");
      auto result = application.create_game(principal, command, key);
      write_snapshot_result(response, std::move(result), 201, true);
    });

    const std::string game_route = R"(/api/v1/games/([A-Za-z0-9_-]{22,64}))";
    server.Get(game_route, [&authenticator, &application](const httplib::Request& request, httplib::Response& response) {
      Principal principal;
      if (!authenticate_request(request, authenticator, false, response, principal))
      {
        return;
      }
      write_snapshot_result(response, application.get_game(principal, path_id(request)), 200);
    });
    server.Post(game_route, [&authenticator, &application](const httplib::Request& request, httplib::Response& response) {
      Principal principal;
      if (!authenticate_request(request, authenticator, true, response, principal))
      {
        return;
      }
      std::uint64_t version = 0;
      if (!parse_expected_version(request, response, version))
      {
        return;
      }
      write_snapshot_result(response,
                            application.join_game(principal, path_id(request), version,
                                                  request.get_header_value("Idempotency-Key")),
                            200);
    });
    server.Get(R"(/api/v1/games/([A-Za-z0-9_-]{22,64})/legal-moves)",
               [&authenticator, &application](const httplib::Request& request, httplib::Response& response) {
      Principal principal;
      if (!authenticate_request(request, authenticator, false, response, principal))
      {
        return;
      }
      auto result = application.get_legal_moves(principal, path_id(request));
      if (std::holds_alternative<ApplicationError>(result))
      {
        write_problem(response, std::get<ApplicationError>(result));
        return;
      }
      const auto& legal_moves = std::get<LegalMoveList>(result);
      response.set_header("ETag", "\"" + std::to_string(legal_moves.version) + "\"");
      Json moves = Json::array();
      for (const auto& move : legal_moves.legal_moves)
      {
        moves.push_back(to_json(move));
      }
      write_json(response, 200, Json{{"version", legal_moves.version}, {"legalMoves", std::move(moves)}});
    });
    server.Get(R"(/api/v1/games/([A-Za-z0-9_-]{22,64})/moves)",
               [&authenticator, &application, config](const httplib::Request& request, httplib::Response& response) {
      Principal principal;
      if (!authenticate_request(request, authenticator, false, response, principal))
      {
        return;
      }
      std::uint64_t after_ply = 0;
      std::uint64_t limit = 100;
      const bool after_ply_ok =
          parse_unsigned_query(request, "afterPly", 0, std::numeric_limits<std::uint64_t>::max(), response, after_ply);
      const bool limit_ok = after_ply_ok
          && parse_unsigned_query(request, "limit", 100, std::min(config.max_history_page_size, std::size_t{200}), response, limit);
      if (!after_ply_ok || !limit_ok)
      {
        return; // error already written by parse_unsigned_query
      }
      if (limit == 0)
      {
        write_problem(response, 400, "invalid_query", "Bad Request", "A query parameter is invalid.");
        return;
      }
      auto result = application.get_move_history(principal, path_id(request), after_ply, limit);
      if (std::holds_alternative<ApplicationError>(result))
      {
        write_problem(response, std::get<ApplicationError>(result));
        return;
      }
      write_json(response, 200, to_json(std::get<MovePage>(result)));
    });
    server.Post(R"(/api/v1/games/([A-Za-z0-9_-]{22,64})/moves)",
                [&authenticator, &application](const httplib::Request& request, httplib::Response& response) {
      Principal principal;
      if (!authenticate_request(request, authenticator, true, response, principal))
      {
        return;
      }
      std::uint64_t version = 0;
      if (!parse_expected_version(request, response, version))
      {
        return;
      }
      Json body;
      if (!parse_json_body(request, response, body))
      {
        return;
      }
      if (!has_only_fields(body, {"uci"}) || !body.contains("uci") || !body["uci"].is_string())
      {
        write_problem(response, 400, "invalid_move_request", "Bad Request", "A UCI move is required.");
        return;
      }
      const std::string uci = body["uci"].get<std::string>();
      const bool valid_length = uci.size() == 4 || uci.size() == 5;
      const bool valid_squares = valid_length && uci[0] >= 'a' && uci[0] <= 'h' && uci[1] >= '1' && uci[1] <= '8'
                                 && uci[2] >= 'a' && uci[2] <= 'h' && uci[3] >= '1' && uci[3] <= '8';
      const bool valid_promotion = uci.size() != 5 || uci[4] == 'q' || uci[4] == 'r' || uci[4] == 'b' || uci[4] == 'n';
      if (!valid_squares || !valid_promotion)
      {
        write_problem(response, 422, "invalid_move", "Unprocessable Content", "The move format is invalid.");
        return;
      }
      MoveCommand command{uci, version};
      write_snapshot_result(response,
                            application.submit_move(principal, path_id(request), command,
                                                    request.get_header_value("Idempotency-Key")),
                            200);
    });

    const auto register_game_action = [&server, &authenticator, &application](const std::string& suffix,
                                                                             auto action) {
      server.Post(R"(/api/v1/games/([A-Za-z0-9_-]{22,64})/)" + suffix,
                  [&authenticator, &application, action](const httplib::Request& request, httplib::Response& response) {
        Principal principal;
        if (!authenticate_request(request, authenticator, true, response, principal))
        {
          return;
        }
        std::uint64_t version = 0;
        if (!parse_expected_version(request, response, version))
        {
          return;
        }
        write_snapshot_result(response,
                              action(application, principal, path_id(request), version,
                                     request.get_header_value("Idempotency-Key")),
                              200);
      });
    };

    register_game_action("resign", [](IGameApplication& app, const Principal& principal, const std::string& game_id,
                                       std::uint64_t version, const std::string& key) {
      return app.resign(principal, game_id, version, key);
    });
    register_game_action("draw-offers", [](IGameApplication& app, const Principal& principal, const std::string& game_id,
                                            std::uint64_t version, const std::string& key) {
      return app.offer_draw(principal, game_id, version, key);
    });
    register_game_action("draw-offers/accept", [](IGameApplication& app, const Principal& principal,
                                                  const std::string& game_id, std::uint64_t version,
                                                  const std::string& key) {
      return app.accept_draw(principal, game_id, version, key);
    });
  }
} // namespace Chess::Backend
