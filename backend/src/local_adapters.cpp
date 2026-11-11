#include "local_adapters.h"

#include "board_manager.h"
#include "game_handler.h"
#include "translation_unit.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>
#include <string_view>
#include <utility>

namespace Chess::Backend
{
  namespace
  {
    using Json = nlohmann::json;

    ApplicationError error(int status, std::string code, std::string title, std::string detail)
    {
      return ApplicationError{status, std::move(code), std::move(title), std::move(detail), std::nullopt};
    }

    std::string random_token(std::size_t length)
    {
      static constexpr std::string_view alphabet =
          "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
      static std::mutex random_mutex;
      static std::random_device random_device;
      std::lock_guard lock(random_mutex);
      std::uniform_int_distribution<std::size_t> distribution(0, alphabet.size() - 1);
      std::string token;
      token.reserve(length);
      for (std::size_t index = 0; index < length; ++index)
      {
        token.push_back(alphabet[distribution(random_device)]);
      }
      return token;
    }

    bool constant_time_equal(std::string_view left, std::string_view right)
    {
      std::size_t difference = left.size() ^ right.size();
      const std::size_t size = std::max(left.size(), right.size());
      for (std::size_t index = 0; index < size; ++index)
      {
        const unsigned char a = index < left.size() ? static_cast<unsigned char>(left[index]) : 0;
        const unsigned char b = index < right.size() ? static_cast<unsigned char>(right[index]) : 0;
        difference |= static_cast<std::size_t>(a ^ b);
      }
      return difference == 0;
    }

    std::string cookie_value(const httplib::Request& request, std::string_view name)
    {
      const std::string cookies = request.get_header_value("Cookie");
      std::size_t start = 0;
      while (start < cookies.size())
      {
        const std::size_t end = cookies.find(';', start);
        std::string_view item(cookies.data() + start,
                              (end == std::string::npos ? cookies.size() : end) - start);
        while (!item.empty() && std::isspace(static_cast<unsigned char>(item.front())))
        {
          item.remove_prefix(1);
        }
        while (!item.empty() && std::isspace(static_cast<unsigned char>(item.back())))
        {
          item.remove_suffix(1);
        }
        const std::size_t equals = item.find('=');
        if (equals != std::string_view::npos && item.substr(0, equals) == name)
        {
          return std::string(item.substr(equals + 1));
        }
        if (end == std::string::npos)
        {
          break;
        }
        start = end + 1;
      }
      return {};
    }

    bool safe_display_name(const std::string& name)
    {
      if (name.empty() || name.size() > 32)
      {
        return false;
      }
      return std::none_of(name.begin(), name.end(), [](unsigned char character) {
        return std::iscntrl(character) != 0;
      });
    }

    void write_local_problem(httplib::Response& response, int status, const char* code, const char* detail)
    {
      response.status = status;
      response.set_header("Cache-Control", "private, no-store");
      response.set_header("X-Content-Type-Options", "nosniff");
      response.set_content(Json{
        {"type", "about:blank"},
        {"title", status == 403 ? "Forbidden" : "Bad Request"},
        {"status", status},
        {"detail", detail},
        {"code", code}
      }.dump(), "application/problem+json; charset=utf-8");
    }

    std::string uci_for_move(const Move& move)
    {
      std::string uci = position_to_algebraic(move.start_pos) + position_to_algebraic(move.end_pos);
      if (move.is_promotion())
      {
        switch (move.promotion_piece)
        {
        case PieceType::KNIGHT: uci += 'n'; break;
        case PieceType::BISHOP: uci += 'b'; break;
        case PieceType::ROOK: uci += 'r'; break;
        case PieceType::QUEEN: uci += 'q'; break;
        default: break;
        }
      }
      return uci;
    }

    std::string color_name(Color color)
    {
      return color == Color::WHITE ? "white" : "black";
    }

    std::string game_status(const ChessGameHandler& handler)
    {
      switch (handler.get_game_state())
      {
      case GameState::CHECKMATE: return "checkmate";
      case GameState::STALEMATE: return "stalemate";
      case GameState::DRAW_INSUFFICIENT_MATERIAL: return "draw_insufficient_material";
      case GameState::DRAW_THREEFOLD_REPETITION: return "draw_threefold_repetition";
      case GameState::DRAW_FIFTY_MOVE_RULE: return "draw_fifty_move_rule";
      case GameState::WHITE_RESIGNED: return "white_resigned";
      case GameState::BLACK_RESIGNED: return "black_resigned";
      case GameState::DRAW_BY_AGREEMENT: return "draw";
      case GameState::WHITE_TIMEOUT: return "white_timeout";
      case GameState::BLACK_TIMEOUT: return "black_timeout";
      case GameState::ONGOING:
      default:
      {
        ChessBoardManager board_manager;
        const Grid& grid = handler.get_current_grid();
        return board_manager.is_king_in_check(grid, grid.current_turn) ? "check" : "ongoing";
      }
      }
    }

    std::string utc_timestamp()
    {
      const std::time_t now = std::time(nullptr);
      std::tm utc{};
#ifdef _WIN32
      gmtime_s(&utc, &now);
#else
      gmtime_r(&now, &utc);
#endif
      std::ostringstream stream;
      stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
      return stream.str();
    }

    Result<GameSnapshot> unavailable_game()
    {
      return error(404, "not_found", "Not Found", "The requested game was not found.");
    }

    Result<GameSnapshot> version_conflict(std::uint64_t version)
    {
      ApplicationError result = error(412, "version_mismatch", "Precondition Failed",
                                      "The game has changed. Refresh the game and retry.");
      result.current_version = version;
      return result;
    }

  } // namespace

  LocalSessionAuthenticator::LocalSessionAuthenticator(std::vector<std::string> allowed_origins)
      : allowed_origins_(std::move(allowed_origins))
  {
  }

  bool LocalSessionAuthenticator::allows_origin(const std::string& origin) const
  {
    return std::find(allowed_origins_.begin(), allowed_origins_.end(), origin) != allowed_origins_.end();
  }

  IssuedGuestSession LocalSessionAuthenticator::issue_guest_session(const std::string& display_name)
  {
    IssuedGuestSession issued{random_token(43), random_token(48)};
    std::lock_guard lock(mutex_);
    sessions_.insert_or_assign(issued.cookie_value,
                               Session{"guest-" + random_token(32), display_name, issued.csrf_token});
    return issued;
  }

  AuthenticationResult LocalSessionAuthenticator::authenticate(const httplib::Request& request,
                                                                bool state_changing) const
  {
    const std::string token = cookie_value(request, "ichess_dev_session");
    if (token.empty())
    {
      return {};
    }

    std::lock_guard lock(mutex_);
    const auto session = sessions_.find(token);
    if (session == sessions_.end())
    {
      return {};
    }
    if (state_changing
        && (!allows_origin(request.get_header_value("Origin"))
            || !constant_time_equal(request.get_header_value("X-CSRF-Token"), session->second.csrf_token)))
    {
      return AuthenticationResult{false, 403, {}, {}};
    }
    return AuthenticationResult{
      true, 401, Principal{session->second.principal_id, session->second.display_name}, session->second.csrf_token
    };
  }

  void register_local_session_routes(httplib::Server& server, LocalSessionAuthenticator& authenticator)
  {
    server.Post("/api/v1/dev/session", [&authenticator](const httplib::Request& request, httplib::Response& response) {
      if (!authenticator.allows_origin(request.get_header_value("Origin")))
      {
        write_local_problem(response, 403, "origin_rejected", "The request origin is not allowed.");
        return;
      }
      std::string content_type = request.get_header_value("Content-Type");
      const std::size_t parameter = content_type.find(';');
      if (parameter != std::string::npos)
      {
        content_type.resize(parameter);
      }
      std::transform(content_type.begin(), content_type.end(), content_type.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
      });
      if (content_type != "application/json")
      {
        write_local_problem(response, 400, "invalid_request", "A JSON request body is required.");
        return;
      }

      Json body;
      try
      {
        body = Json::parse(request.body);
      }
      catch (const Json::parse_error&)
      {
        write_local_problem(response, 400, "invalid_json", "The request body is not valid JSON.");
        return;
      }
      if (!body.is_object() || body.size() != 1 || !body.contains("displayName")
          || !body["displayName"].is_string() || !safe_display_name(body["displayName"].get<std::string>()))
      {
        write_local_problem(response, 400, "invalid_display_name", "Choose a display name of 1 to 32 characters.");
        return;
      }

      const IssuedGuestSession session =
          authenticator.issue_guest_session(body["displayName"].get<std::string>());
      response.status = 201;
      response.set_header("Cache-Control", "private, no-store");
      response.set_header("X-Content-Type-Options", "nosniff");
      response.set_header("Referrer-Policy", "no-referrer");
      response.set_header("Set-Cookie",
                          "ichess_dev_session=" + session.cookie_value + "; Path=/; HttpOnly; SameSite=Strict");
      response.set_content(Json{{"csrfToken", session.csrf_token}}.dump(), "application/json; charset=utf-8");
    });
  }

  struct InMemoryGameApplication::GameRecord
  {
      std::string id;
      std::string white_principal;
      std::string black_principal;
    std::string white_display_name;
    std::string black_display_name;
    ChessGameHandler handler;
      std::vector<MoveRecord> history;
      std::uint64_t version = 1;
      std::optional<Color> draw_offered_by;
  };

  InMemoryGameApplication::InMemoryGameApplication() = default;
  InMemoryGameApplication::~InMemoryGameApplication() = default;

  bool InMemoryGameApplication::is_ready() const { return true; }

  std::string InMemoryGameApplication::idempotency_map_key(const Principal& principal,
                                                           const std::string& operation,
                                                           const std::string& key) const
  {
    return principal.id + '\n' + operation + '\n' + key;
  }

  Result<GameSnapshot> InMemoryGameApplication::cached_result(const Principal& principal,
                                                              const std::string& operation,
                                                              const std::string& key,
                                                              const std::string& fingerprint) const
  {
    const auto found = idempotency_.find(idempotency_map_key(principal, operation, key));
    if (found == idempotency_.end())
    {
      return error(404, "idempotency_miss", "Not Found", "No cached operation was found.");
    }
    if (found->second.fingerprint != fingerprint)
    {
      return error(409, "idempotency_conflict", "Conflict",
                   "The idempotency key was already used for a different request.");
    }
    return found->second.snapshot;
  }

  void InMemoryGameApplication::remember_result(const Principal& principal,
                                                const std::string& operation,
                                                const std::string& key,
                                                const std::string& fingerprint,
                                                const GameSnapshot& snapshot)
  {
    idempotency_.insert_or_assign(idempotency_map_key(principal, operation, key),
                                  IdempotencyEntry{fingerprint, snapshot});
  }

  InMemoryGameApplication::GameRecord* InMemoryGameApplication::authorized_game(const Principal& principal,
                                                                                 const std::string& game_id)
  {
    const auto found = games_.find(game_id);
    if (found == games_.end()
        || (found->second->white_principal != principal.id && found->second->black_principal != principal.id))
    {
      return nullptr;
    }
    return found->second.get();
  }

  const InMemoryGameApplication::GameRecord* InMemoryGameApplication::authorized_game(
      const Principal& principal, const std::string& game_id) const
  {
    const auto found = games_.find(game_id);
    if (found == games_.end()
        || (found->second->white_principal != principal.id && found->second->black_principal != principal.id))
    {
      return nullptr;
    }
    return found->second.get();
  }

  GameSnapshot InMemoryGameApplication::make_snapshot(const GameRecord& game, const Principal& viewer) const
  {
    GameSnapshot snapshot;
    snapshot.game_id = game.id;
    snapshot.version = game.version;
    snapshot.fen = game.handler.get_current_fen();
    snapshot.side_to_move = color_name(game.handler.get_current_grid().current_turn);
    snapshot.status = game_status(game.handler);
    snapshot.move_count = game.history.size();
    snapshot.viewer_seat = viewer.id == game.white_principal ? "white" : "black";
    snapshot.white_player.display_name = game.white_display_name;
    if (!game.black_principal.empty())
    {
      snapshot.black_player = Player{game.black_display_name};
    }
    snapshot.server_time = utc_timestamp();
    ChessTranslationUnit translation;
    for (const Move& move : game.handler.get_legal_moves())
    {
      snapshot.legal_moves.push_back(LegalMove{
        uci_for_move(move),
        translation.move_to_san(move, game.handler.get_current_grid())
      });
    }
    return snapshot;
  }

  Result<GameSnapshot> InMemoryGameApplication::create_game(const Principal& principal,
                                                            const CreateGameCommand& command,
                                                            const std::string& idempotency_key)
  {
    std::lock_guard lock(mutex_);
    const std::string fingerprint = command.initial_fen.value_or("<standard>") + "|"
                                    + (command.time_control.has_value() ? "time-control" : "no-time-control");
    const std::string cache_key = idempotency_map_key(principal, "create", idempotency_key);
    if (const auto found = idempotency_.find(cache_key); found != idempotency_.end())
    {
      if (found->second.fingerprint != fingerprint)
      {
        return error(409, "idempotency_conflict", "Conflict",
                     "The idempotency key was already used for a different request.");
      }
      return found->second.snapshot;
    }
    if (command.time_control.has_value())
    {
      return error(422, "time_control_unsupported", "Unprocessable Content",
                   "Time controls are not available in the local development service.");
    }

    auto game = std::make_unique<GameRecord>();
    game->id = random_token(32);
    game->white_principal = principal.id;
    game->white_display_name = principal.display_name.empty() ? "White" : principal.display_name;
    game->handler.start_new_game();
    try
    {
      if (command.initial_fen.has_value())
      {
        game->handler.load_game_from_fen(*command.initial_fen);
      }
    }
    catch (const std::exception&)
    {
      return error(422, "invalid_fen", "Unprocessable Content", "The initial position is invalid.");
    }
    GameRecord& stored = *game;
    games_.emplace(game->id, std::move(game));
    GameSnapshot snapshot = make_snapshot(stored, principal);
    remember_result(principal, "create", idempotency_key, fingerprint, snapshot);
    return snapshot;
  }

  Result<GameSnapshot> InMemoryGameApplication::get_game(const Principal& principal, const std::string& game_id) const
  {
    std::lock_guard lock(mutex_);
    const GameRecord* game = authorized_game(principal, game_id);
    if (game == nullptr)
    {
      return unavailable_game();
    }
    return make_snapshot(*game, principal);
  }

  Result<GameSnapshot> InMemoryGameApplication::join_game(const Principal& principal,
                                                          const std::string& game_id,
                                                          std::uint64_t expected_version,
                                                          const std::string& idempotency_key)
  {
    std::lock_guard lock(mutex_);
    const std::string fingerprint = game_id + "|" + std::to_string(expected_version);
    const std::string cache_key = idempotency_map_key(principal, "join", idempotency_key);
    if (const auto found = idempotency_.find(cache_key); found != idempotency_.end())
    {
      if (found->second.fingerprint != fingerprint)
      {
        return error(409, "idempotency_conflict", "Conflict",
                     "The idempotency key was already used for a different request.");
      }
      return found->second.snapshot;
    }
    const auto found = games_.find(game_id);
    if (found == games_.end())
    {
      return unavailable_game();
    }
    GameRecord& game = *found->second;
    if (game.black_principal == principal.id)
    {
      return make_snapshot(game, principal);
    }
    if (game.white_principal == principal.id)
    {
      return error(409, "seat_already_occupied", "Conflict", "The caller already occupies the White seat.");
    }
    if (!game.black_principal.empty())
    {
      return unavailable_game();
    }
    if (expected_version != game.version)
    {
      return version_conflict(game.version);
    }
    game.black_principal = principal.id;
    game.black_display_name = principal.display_name.empty() ? "Black" : principal.display_name;
    ++game.version;
    GameSnapshot snapshot = make_snapshot(game, principal);
    remember_result(principal, "join", idempotency_key, fingerprint, snapshot);
    return snapshot;
  }

  Result<LegalMoveList> InMemoryGameApplication::get_legal_moves(const Principal& principal,
                                                                const std::string& game_id) const
  {
    std::lock_guard lock(mutex_);
    const GameRecord* game = authorized_game(principal, game_id);
    if (game == nullptr)
    {
      return error(404, "not_found", "Not Found", "The requested game was not found.");
    }
    LegalMoveList result;
    result.version = game->version;
    ChessTranslationUnit translation;
    for (const Move& move : game->handler.get_legal_moves())
    {
      result.legal_moves.push_back(
        LegalMove{uci_for_move(move), translation.move_to_san(move, game->handler.get_current_grid())});
    }
    return result;
  }

  Result<MovePage> InMemoryGameApplication::get_move_history(const Principal& principal,
                                                             const std::string& game_id,
                                                             std::uint64_t after_ply,
                                                             std::uint64_t limit) const
  {
    std::lock_guard lock(mutex_);
    const GameRecord* game = authorized_game(principal, game_id);
    if (game == nullptr)
    {
      return error(404, "not_found", "Not Found", "The requested game was not found.");
    }
    MovePage result;
    const std::size_t start = static_cast<std::size_t>(std::min<std::uint64_t>(after_ply, game->history.size()));
    const std::size_t end = std::min(game->history.size(), start + static_cast<std::size_t>(limit));
    result.moves.insert(result.moves.end(), game->history.begin() + static_cast<std::ptrdiff_t>(start),
                        game->history.begin() + static_cast<std::ptrdiff_t>(end));
    if (end < game->history.size())
    {
      result.next_after_ply = result.moves.back().ply;
    }
    return result;
  }

  Result<GameSnapshot> InMemoryGameApplication::submit_move(const Principal& principal,
                                                            const std::string& game_id,
                                                            const MoveCommand& command,
                                                            const std::string& idempotency_key)
  {
    std::lock_guard lock(mutex_);
    const std::string fingerprint = game_id + "|" + command.uci + "|" + std::to_string(command.expected_version);
    const std::string cache_key = idempotency_map_key(principal, "move", idempotency_key);
    if (const auto cached = idempotency_.find(cache_key); cached != idempotency_.end())
    {
      if (cached->second.fingerprint != fingerprint)
      {
        return error(409, "idempotency_conflict", "Conflict",
                     "The idempotency key was already used for a different request.");
      }
      return cached->second.snapshot;
    }
    GameRecord* game = authorized_game(principal, game_id);
    if (game == nullptr)
    {
      return unavailable_game();
    }
    if (command.expected_version != game->version)
    {
      return version_conflict(game->version);
    }
    const Color seat = game->white_principal == principal.id ? Color::WHITE : Color::BLACK;
    if (game->black_principal.empty() || game->handler.get_current_grid().current_turn != seat)
    {
      return error(403, "wrong_turn", "Forbidden", "The caller cannot move for the current side.");
    }
    if (game->handler.get_game_state() != GameState::ONGOING)
    {
      return error(409, "game_finished", "Conflict", "The game is already finished.");
    }

    const std::vector<Move> legal_moves = game->handler.get_legal_moves();
    const auto legal = std::find_if(legal_moves.begin(), legal_moves.end(), [&command](const Move& move) {
      return uci_for_move(move) == command.uci;
    });
    if (legal == legal_moves.end())
    {
      return error(422, "illegal_move", "Unprocessable Content", "The move is not legal in the current position.");
    }

    ChessTranslationUnit translation;
    const std::string san = translation.move_to_san(*legal, game->handler.get_current_grid());
    if (!game->handler.make_move(*legal))
    {
      return error(422, "illegal_move", "Unprocessable Content", "The move could not be applied.");
    }
    game->history.push_back(MoveRecord{game->history.size() + 1, command.uci, san});
    ++game->version;
    GameSnapshot snapshot = make_snapshot(*game, principal);
    remember_result(principal, "move", idempotency_key, fingerprint, snapshot);
    return snapshot;
  }

  Result<GameSnapshot> InMemoryGameApplication::resign(const Principal& principal,
                                                       const std::string& game_id,
                                                       std::uint64_t expected_version,
                                                       const std::string& idempotency_key)
  {
    std::lock_guard lock(mutex_);
    const std::string fingerprint = game_id + "|" + std::to_string(expected_version);
    const std::string cache_key = idempotency_map_key(principal, "resign", idempotency_key);
    if (const auto cached = idempotency_.find(cache_key); cached != idempotency_.end())
    {
      if (cached->second.fingerprint != fingerprint)
      {
        return error(409, "idempotency_conflict", "Conflict",
                     "The idempotency key was already used for a different request.");
      }
      return cached->second.snapshot;
    }
    GameRecord* game = authorized_game(principal, game_id);
    if (game == nullptr)
    {
      return unavailable_game();
    }
    if (expected_version != game->version)
    {
      return version_conflict(game->version);
    }
    const Color seat = game->white_principal == principal.id ? Color::WHITE : Color::BLACK;
    if (!game->handler.resign(seat))
    {
      return error(409, "game_finished", "Conflict", "The game is already finished.");
    }
    ++game->version;
    GameSnapshot snapshot = make_snapshot(*game, principal);
    remember_result(principal, "resign", idempotency_key, fingerprint, snapshot);
    return snapshot;
  }

  Result<GameSnapshot> InMemoryGameApplication::offer_draw(const Principal& principal,
                                                           const std::string& game_id,
                                                           std::uint64_t expected_version,
                                                           const std::string& idempotency_key)
  {
    std::lock_guard lock(mutex_);
    const std::string fingerprint = game_id + "|" + std::to_string(expected_version);
    const std::string cache_key = idempotency_map_key(principal, "offer-draw", idempotency_key);
    if (const auto cached = idempotency_.find(cache_key); cached != idempotency_.end())
    {
      if (cached->second.fingerprint != fingerprint)
      {
        return error(409, "idempotency_conflict", "Conflict",
                     "The idempotency key was already used for a different request.");
      }
      return cached->second.snapshot;
    }
    GameRecord* game = authorized_game(principal, game_id);
    if (game == nullptr)
    {
      return unavailable_game();
    }
    if (expected_version != game->version)
    {
      return version_conflict(game->version);
    }
    if (game->black_principal.empty())
    {
      return error(409, "opponent_not_joined", "Conflict", "A draw cannot be offered before the opponent joins.");
    }
    const Color seat = game->white_principal == principal.id ? Color::WHITE : Color::BLACK;
    if (game->draw_offered_by == seat)
    {
      return error(409, "draw_already_offered", "Conflict", "The caller already has an active draw offer.");
    }
    if (!game->handler.offer_draw(seat))
    {
      return error(409, "game_finished", "Conflict", "The game is already finished.");
    }
    game->draw_offered_by = seat;
    ++game->version;
    GameSnapshot snapshot = make_snapshot(*game, principal);
    remember_result(principal, "offer-draw", idempotency_key, fingerprint, snapshot);
    return snapshot;
  }

  Result<GameSnapshot> InMemoryGameApplication::accept_draw(const Principal& principal,
                                                            const std::string& game_id,
                                                            std::uint64_t expected_version,
                                                            const std::string& idempotency_key)
  {
    std::lock_guard lock(mutex_);
    const std::string fingerprint = game_id + "|" + std::to_string(expected_version);
    const std::string cache_key = idempotency_map_key(principal, "accept-draw", idempotency_key);
    if (const auto cached = idempotency_.find(cache_key); cached != idempotency_.end())
    {
      if (cached->second.fingerprint != fingerprint)
      {
        return error(409, "idempotency_conflict", "Conflict",
                     "The idempotency key was already used for a different request.");
      }
      return cached->second.snapshot;
    }
    GameRecord* game = authorized_game(principal, game_id);
    if (game == nullptr)
    {
      return unavailable_game();
    }
    if (expected_version != game->version)
    {
      return version_conflict(game->version);
    }
    const Color seat = game->white_principal == principal.id ? Color::WHITE : Color::BLACK;
    if (!game->draw_offered_by.has_value() || *game->draw_offered_by == seat
        || !game->handler.accept_draw(seat))
    {
      return error(409, "no_draw_offer", "Conflict", "There is no opponent draw offer to accept.");
    }
    game->draw_offered_by.reset();
    ++game->version;
    GameSnapshot snapshot = make_snapshot(*game, principal);
    remember_result(principal, "accept-draw", idempotency_key, fingerprint, snapshot);
    return snapshot;
  }
} // namespace Chess::Backend
