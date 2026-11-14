#include "sqlite_game_application.h"

#include "execution_budget.h"
#include "game_handler.h"
#include "translation_unit.h"
#include <nlohmann/json.hpp>
#include <array>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif

namespace Chess::Backend
{
  namespace
  {
    using Json = nlohmann::json;

    class Statement
    {
      public:
        Statement(sqlite3* database, const char* sql)
        {
          ExecutionBudget::checkpoint();
          if (sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr) != SQLITE_OK)
          {
            statement_ = nullptr;
          }
        }

        ~Statement() { sqlite3_finalize(statement_); }
        Statement(const Statement&) = delete;
        Statement& operator=(const Statement&) = delete;

        sqlite3_stmt* get() const { return statement_; }
        explicit operator bool() const { return statement_ != nullptr; }

      private:
        sqlite3_stmt* statement_ = nullptr;
    };

    class Transaction
    {
      public:
        explicit Transaction(sqlite3* database, bool write = true)
          : database_(database), active_(false)
        {
          ExecutionBudget::checkpoint();
          active_ = sqlite3_exec(database, write ? "BEGIN IMMEDIATE" : "BEGIN", nullptr, nullptr, nullptr) == SQLITE_OK;
        }
        ~Transaction()
        {
          if (active_)
          {
            sqlite3_exec(database_, "ROLLBACK", nullptr, nullptr, nullptr);
          }
        }
        bool commit()
        {
          ExecutionBudget::checkpoint();
          if (!active_ || sqlite3_exec(database_, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK)
          {
            return false;
          }
          active_ = false;
          return true;
        }
        explicit operator bool() const { return active_; }

      private:
        sqlite3* database_;
        bool active_;
    };

    bool bind_text(sqlite3_stmt* statement, int index, const std::string& value)
    {
      return sqlite3_bind_text(statement, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT) == SQLITE_OK;
    }

    std::string column_text(sqlite3_stmt* statement, int column)
    {
      const auto* value = sqlite3_column_text(statement, column);
      return value == nullptr ? std::string{}
                              : std::string(reinterpret_cast<const char*>(value), sqlite3_column_bytes(statement, column));
    }

    ApplicationError error(int status, std::string code, std::string title, std::string detail,
                           std::optional<std::uint64_t> version = std::nullopt)
    {
      return ApplicationError{status, std::move(code), std::move(title), std::move(detail), version};
    }

    ApplicationError internal_error()
    {
      return error(500, "internal_error", "Internal Server Error", "The request could not be completed.");
    }

    ApplicationError storage_unavailable()
    {
      return error(503, "storage_unavailable", "Service Unavailable", "Game storage is unavailable.");
    }

    std::optional<std::string> random_game_id()
    {
      std::array<unsigned char, 16> bytes{};
#ifdef _WIN32
      if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
      {
        return std::nullopt;
      }
#else
      std::ifstream source("/dev/urandom", std::ios::binary);
      if (!source.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
      {
        return std::nullopt;
      }
#endif
      constexpr char digits[] = "0123456789abcdef";
      std::string result;
      result.reserve(32);
      for (const unsigned char byte : bytes)
      {
        result.push_back(digits[byte >> 4]);
        result.push_back(digits[byte & 15]);
      }
      return result;
    }

    std::string utc_now()
    {
      const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      std::tm value{};
#ifdef _WIN32
      gmtime_s(&value, &now);
#else
      gmtime_r(&now, &value);
#endif
      std::ostringstream result;
      result << std::put_time(&value, "%Y-%m-%dT%H:%M:%SZ");
      return result.str();
    }

    std::int64_t epoch_milliseconds()
    {
      return std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count();
    }

    std::string status_name(Chess::GameState state)
    {
      switch (state)
      {
      case Chess::GameState::ONGOING: return "ongoing";
      case Chess::GameState::CHECKMATE: return "checkmate";
      case Chess::GameState::STALEMATE: return "stalemate";
      case Chess::GameState::DRAW_INSUFFICIENT_MATERIAL: return "draw_insufficient_material";
      case Chess::GameState::DRAW_THREEFOLD_REPETITION: return "draw_threefold_repetition";
      case Chess::GameState::DRAW_FIFTY_MOVE_RULE: return "draw_fifty_move_rule";
      case Chess::GameState::WHITE_RESIGNED: return "white_resigned";
      case Chess::GameState::BLACK_RESIGNED: return "black_resigned";
      case Chess::GameState::DRAW_BY_AGREEMENT: return "draw_by_agreement";
      case Chess::GameState::WHITE_TIMEOUT: return "white_timeout";
      case Chess::GameState::BLACK_TIMEOUT: return "black_timeout";
      }
      return "ongoing";
    }

    bool valid_initial_position(const Chess::Grid& grid)
    {
      std::array<int, 2> kings{};
      std::array<int, 2> pawns{};
      std::array<int, 2> pieces{};
      for (const auto& piece : grid.get_all_piece_properties())
      {
        ExecutionBudget::checkpoint();
        const std::size_t color = piece.color == Chess::Color::WHITE ? 0 : 1;
        ++pieces[color];
        if (piece.type == Chess::PieceType::KING)
        {
          ++kings[color];
        }
        if (piece.type == Chess::PieceType::PAWN)
        {
          ++pawns[color];
          if (piece.position.rank == 0 || piece.position.rank == 7)
          {
            return false;
          }
        }
      }
      if (kings[0] != 1 || kings[1] != 1 || pawns[0] > 8 || pawns[1] > 8
          || pieces[0] > 16 || pieces[1] > 16 || grid.flags.halfmove_clock < 0
          || grid.flags.fullmove_number >= (std::numeric_limits<int>::max)())
      {
        return false;
      }
      Chess::ChessBoardManager board;
      const auto previous_side = grid.current_turn == Chess::Color::WHITE ? Chess::Color::BLACK : Chess::Color::WHITE;
      return !board.is_king_in_check(grid, previous_side);
    }

    bool consistent_fen(const std::string& fen, const Chess::Grid& grid)
    {
      std::istringstream fields(fen);
      std::string placement, turn, rights, target;
      int halfmove = 0;
      if (!(fields >> placement >> turn >> rights >> target >> halfmove))
      {
        return false;
      }
      const auto has_piece = [&grid](int file, int rank, Chess::PieceType type, Chess::Color color)
      {
        const auto piece = grid.get_piece(Chess::Position(file, rank));
        return piece && piece->type == type && piece->color == color;
      };
      for (const char right : rights)
      {
        if (right == '-')
        {
          continue;
        }
        const bool white = right == 'K' || right == 'Q';
        const auto color = white ? Chess::Color::WHITE : Chess::Color::BLACK;
        const int rank = white ? 0 : 7;
        const int rook_file = right == 'K' || right == 'k' ? 7 : 0;
        if (!has_piece(4, rank, Chess::PieceType::KING, color)
            || !has_piece(rook_file, rank, Chess::PieceType::ROOK, color))
        {
          return false;
        }
      }
      if (target != "-")
      {
        const bool white_turn = grid.current_turn == Chess::Color::WHITE;
        if (target.size() != 2 || target[0] < 'a' || target[0] > 'h'
            || target[1] != (white_turn ? '6' : '3') || halfmove != 0)
        {
          return false;
        }
        const int file = target[0] - 'a';
        if (grid.is_occupied(Chess::Position(file, white_turn ? 5 : 2))
            || grid.is_occupied(Chess::Position(file, white_turn ? 6 : 1))
            || !has_piece(file, white_turn ? 4 : 3, Chess::PieceType::PAWN,
                          white_turn ? Chess::Color::BLACK : Chess::Color::WHITE))
        {
          return false;
        }
      }
      return true;
    }

    std::string initial_status(const Chess::Grid& grid)
    {
      ExecutionBudget::checkpoint();
      Chess::ChessBoardManager board;
      if (board.is_checkmate(grid, grid.current_turn))
      {
        return "checkmate";
      }
      ExecutionBudget::checkpoint();
      if (board.is_stalemate(grid, grid.current_turn))
      {
        return "stalemate";
      }
      ExecutionBudget::checkpoint();
      switch (board.get_draw_reason(grid, grid.current_turn))
      {
      case Chess::DrawReason::INSUFFICIENT_MATERIAL: return "draw_insufficient_material";
      case Chess::DrawReason::FIFTY_MOVE_RULE: return "draw_fifty_move_rule";
      case Chess::DrawReason::NONE: return "ongoing";
      }
      return "ongoing";
    }

    constexpr std::uint64_t maximum_plies = 4096;

    Result<GameSnapshot> load_snapshot(sqlite3* database, const Principal& principal, const std::string& game_id,
                       Chess::ChessGameHandler* restored = nullptr)
    {
      ExecutionBudget::checkpoint();
      Statement game_query(database,
                           "SELECT white_principal, black_principal, initial_fen, version, status, updated_at, "
                           "initial_ms, increment_ms, clock_white_ms, clock_black_ms, clock_started_at_ms "
                           "FROM games WHERE game_id = ?1");
      if (!game_query || !bind_text(game_query.get(), 1, game_id))
      {
        return internal_error();
      }
      const int game_step = sqlite3_step(game_query.get());
      if (game_step == SQLITE_DONE)
      {
        return error(404, "game_not_found", "Not Found", "The requested game was not found.");
      }
      if (game_step != SQLITE_ROW)
      {
        return internal_error();
      }
      const std::string white_principal = column_text(game_query.get(), 0);
      const std::string black_principal = column_text(game_query.get(), 1);
      const bool is_white = principal.id == white_principal;
      const bool is_black = !black_principal.empty() && principal.id == black_principal;
      if (!is_white && !is_black)
      {
        return error(404, "game_not_found", "Not Found", "The requested game was not found.");
      }

      const std::string initial_fen = column_text(game_query.get(), 2);
      if (sqlite3_column_type(game_query.get(), 3) != SQLITE_INTEGER
          || sqlite3_column_int64(game_query.get(), 3) <= 0)
      {
        return internal_error();
      }
      const std::uint64_t version = static_cast<std::uint64_t>(sqlite3_column_int64(game_query.get(), 3));
      const std::string persisted_status = column_text(game_query.get(), 4);
      const auto initial_ms_type = sqlite3_column_type(game_query.get(), 6);
      const auto white_clock_type = sqlite3_column_type(game_query.get(), 8);
      const auto black_clock_type = sqlite3_column_type(game_query.get(), 9);
      const auto started_type = sqlite3_column_type(game_query.get(), 10);
      const bool timed = initial_ms_type != SQLITE_NULL;
        const auto persisted_white_clock = timed ? sqlite3_column_int64(game_query.get(), 8) : 0;
        const auto persisted_black_clock = timed ? sqlite3_column_int64(game_query.get(), 9) : 0;
        const auto persisted_clock_started = started_type == SQLITE_INTEGER
          ? sqlite3_column_int64(game_query.get(), 10) : 0;
      if (timed && (initial_ms_type != SQLITE_INTEGER
          || sqlite3_column_type(game_query.get(), 7) != SQLITE_INTEGER
          || white_clock_type != SQLITE_INTEGER || black_clock_type != SQLITE_INTEGER
          || sqlite3_column_int64(game_query.get(), 6) <= 0
          || sqlite3_column_int64(game_query.get(), 7) != 0
          || sqlite3_column_int64(game_query.get(), 8) < 0
          || sqlite3_column_int64(game_query.get(), 9) < 0
          || (started_type != SQLITE_NULL && started_type != SQLITE_INTEGER)))
        return internal_error();
      if (!timed && (white_clock_type != SQLITE_NULL || black_clock_type != SQLITE_NULL || started_type != SQLITE_NULL))
        return internal_error();
      const std::array<std::string_view, 11> statuses{
        "ongoing", "checkmate", "stalemate", "draw_insufficient_material", "draw_threefold_repetition",
        "draw_fifty_move_rule", "white_resigned", "black_resigned", "draw_by_agreement", "white_timeout", "black_timeout"};
      bool known_status = false;
      for (const auto candidate : statuses)
      {
        known_status = known_status || candidate == persisted_status;
      }
      if (!known_status)
      {
        return internal_error();
      }
      if (sqlite3_step(game_query.get()) != SQLITE_DONE)
      {
        return internal_error();
      }
      if (initial_fen.empty() || initial_fen.size() > 128)
      {
        return internal_error();
      }

      Chess::ChessGameHandler local_handler;
      auto& handler = restored ? *restored : local_handler;
      Chess::ChessTranslationUnit translator;
      try
      {
        handler.load_game_from_fen(initial_fen);
        if (handler.get_current_fen() != initial_fen || !valid_initial_position(handler.get_current_grid())
            || !consistent_fen(initial_fen, handler.get_current_grid()))
        {
          return internal_error();
        }
      }
      catch (const ExecutionDeadlineExceeded&)
      {
        throw;
      }
      catch (...)
      {
        return internal_error();
      }

      Statement last_move(database, "SELECT ply FROM game_moves WHERE game_id = ?1 ORDER BY ply DESC LIMIT 1");
      if (!last_move || !bind_text(last_move.get(), 1, game_id))
      {
        return internal_error();
      }
      const int last_step = sqlite3_step(last_move.get());
      if (last_step == SQLITE_ROW)
      {
        if (sqlite3_column_type(last_move.get(), 0) != SQLITE_INTEGER)
        {
          return internal_error();
        }
        if (sqlite3_column_int64(last_move.get(), 0) > static_cast<sqlite3_int64>(maximum_plies))
        {
          return error(409, "history_limit", "Conflict", "The game exceeds the supported move history limit.");
        }
        if (sqlite3_step(last_move.get()) != SQLITE_DONE)
        {
          return internal_error();
        }
      }
      else if (last_step != SQLITE_DONE)
      {
        return internal_error();
      }
      Statement move_query(database, "SELECT uci, ply FROM game_moves WHERE game_id = ?1 ORDER BY ply LIMIT 4097");
      if (!move_query || !bind_text(move_query.get(), 1, game_id))
      {
        return internal_error();
      }
      std::uint64_t move_count = 0;
      int move_step = SQLITE_OK;
      while ((move_step = sqlite3_step(move_query.get())) == SQLITE_ROW)
      {
        ExecutionBudget::checkpoint();
        if (move_count >= maximum_plies)
        {
          return error(409, "history_limit", "Conflict", "The game exceeds the supported move history limit.");
        }
        if (sqlite3_column_type(move_query.get(), 1) != SQLITE_INTEGER
            || sqlite3_column_int64(move_query.get(), 1) != static_cast<sqlite3_int64>(move_count + 1))
        {
          return internal_error();
        }
        const std::string uci = column_text(move_query.get(), 0);
        if (handler.get_current_grid().flags.halfmove_clock >= (std::numeric_limits<int>::max)() - 1
            || handler.get_current_grid().flags.fullmove_number >= (std::numeric_limits<int>::max)() - 1)
        {
          return error(409, "counter_limit", "Conflict", "The position counter limit has been reached.");
        }
        bool applied = false;
        for (const auto& move : handler.get_legal_moves())
        {
          ExecutionBudget::checkpoint();
          if (translator.move_to_algebraic(move, handler.get_current_grid()) == uci)
          {
            applied = handler.make_move(move);
            break;
          }
        }
        if (!applied)
        {
          return internal_error();
        }
        ++move_count;
      }
      ExecutionBudget::checkpoint();
      if (move_step != SQLITE_DONE)
      {
        return internal_error();
      }

      GameSnapshot snapshot;
      snapshot.game_id = game_id;
      snapshot.version = version;
      snapshot.fen = handler.get_current_fen();
      snapshot.side_to_move = handler.get_current_grid().current_turn == Chess::Color::WHITE ? "white" : "black";
      snapshot.status = persisted_status == "ongoing"
                ? (move_count == 0 ? initial_status(handler.get_current_grid()) : status_name(handler.get_game_state()))
                : persisted_status;
      snapshot.move_count = move_count;
      snapshot.viewer_seat = is_white ? "white" : "black";
      snapshot.white_player.display_name = "White";
      if (!black_principal.empty())
      {
        snapshot.black_player = Player{"Black"};
      }
      snapshot.server_time = utc_now();
      if (timed)
      {
        auto white_remaining = persisted_white_clock;
        auto black_remaining = persisted_black_clock;
        std::optional<std::string> running_color;
        if (persisted_status == "ongoing" && started_type == SQLITE_INTEGER)
        {
          const auto elapsed = (std::max<std::int64_t>)(0, epoch_milliseconds() - persisted_clock_started);
          auto& remaining = handler.get_current_grid().current_turn == Chess::Color::WHITE
              ? white_remaining : black_remaining;
          remaining = (std::max<std::int64_t>)(0, remaining - elapsed);
          running_color = handler.get_current_grid().current_turn == Chess::Color::WHITE ? "white" : "black";
        }
        snapshot.clock = ClockSnapshot{static_cast<std::uint64_t>(white_remaining),
            static_cast<std::uint64_t>(black_remaining), std::move(running_color), 0};
      }
      if (snapshot.status == "ongoing")
      {
        for (const auto& move : handler.get_legal_moves())
        {
          ExecutionBudget::checkpoint();
          snapshot.legal_moves.push_back(LegalMove{translator.move_to_algebraic(move, handler.get_current_grid()),
                                                   translator.move_to_san(move, handler.get_current_grid())});
        }
      }
      ExecutionBudget::checkpoint();
      return snapshot;
    }

    Json snapshot_json(const GameSnapshot& snapshot)
    {
      Json legal_moves = Json::array();
      for (const auto& move : snapshot.legal_moves)
      {
        ExecutionBudget::checkpoint();
        legal_moves.push_back(Json{{"uci", move.uci}, {"san", move.san}});
      }
      Json clock = nullptr;
      if (snapshot.clock)
      {
        clock = Json{{"whiteRemainingMs", snapshot.clock->white_remaining_ms},
                     {"blackRemainingMs", snapshot.clock->black_remaining_ms},
                     {"runningColor", snapshot.clock->running_color ? Json(*snapshot.clock->running_color) : Json(nullptr)},
                     {"incrementMs", snapshot.clock->increment_ms}};
      }
      return Json{{"gameId", snapshot.game_id}, {"version", snapshot.version}, {"fen", snapshot.fen},
                  {"sideToMove", snapshot.side_to_move}, {"status", snapshot.status},
                  {"legalMoves", std::move(legal_moves)}, {"moveCount", snapshot.move_count},
                  {"viewerSeat", snapshot.viewer_seat},
                  {"players", {{"white", {{"displayName", snapshot.white_player.display_name}}},
                               {"black", snapshot.black_player ? Json{{"displayName", snapshot.black_player->display_name}} : Json(nullptr)}}},
                  {"clock", std::move(clock)}, {"serverTime", snapshot.server_time}};
    }

    GameSnapshot snapshot_from_json(const Json& value)
    {
      GameSnapshot snapshot;
      snapshot.game_id = value.at("gameId").get<std::string>();
      snapshot.version = value.at("version").get<std::uint64_t>();
      snapshot.fen = value.at("fen").get<std::string>();
      snapshot.side_to_move = value.at("sideToMove").get<std::string>();
      snapshot.status = value.at("status").get<std::string>();
      snapshot.move_count = value.at("moveCount").get<std::uint64_t>();
      snapshot.viewer_seat = value.at("viewerSeat").get<std::string>();
      snapshot.white_player.display_name = value.at("players").at("white").at("displayName").get<std::string>();
      if (!value.at("players").at("black").is_null())
      {
        snapshot.black_player = Player{value.at("players").at("black").at("displayName").get<std::string>()};
      }
      snapshot.server_time = value.at("serverTime").get<std::string>();
      if (!value.at("clock").is_null())
      {
        const auto& clock = value.at("clock");
        std::optional<std::string> running_color;
        if (!clock.at("runningColor").is_null()) running_color = clock.at("runningColor").get<std::string>();
        snapshot.clock = ClockSnapshot{clock.at("whiteRemainingMs").get<std::uint64_t>(),
        clock.at("blackRemainingMs").get<std::uint64_t>(), std::move(running_color),
        clock.at("incrementMs").get<std::uint64_t>()};
      }
      for (const auto& move : value.at("legalMoves"))
      {
        ExecutionBudget::checkpoint();
        snapshot.legal_moves.push_back(LegalMove{move.at("uci").get<std::string>(), move.at("san").get<std::string>()});
      }
      return snapshot;
    }

    std::string create_fingerprint(const std::string& fen, const std::optional<TimeControl>& time_control)
    {
      return std::to_string(fen.size()) + ":" + fen + ":"
          + (time_control ? std::to_string(time_control->initial_ms) + ":" + std::to_string(time_control->increment_ms)
                          : "untimed");
    }

    enum class Operation { join, move, resign, offer_draw, accept_draw };

    Result<GameSnapshot> mutate(SqliteDatabase& owner, const Principal& principal, const std::string& game_id,
                                 std::uint64_t expected_version, const std::string& key, Operation operation,
                                 const std::string& uci = {})
    {
      ExecutionBudget::checkpoint();
      if (!owner.is_ready())
      {
        return storage_unavailable();
      }
      if (principal.id.empty() || principal.id.size() > 256 || game_id.empty() || game_id.size() > 256
          || key.size() < 16 || key.size() > 128)
      {
        return error(400, "invalid_request", "Bad Request", "The request could not be completed.");
      }
      if (operation == Operation::move
          && ((uci.size() != 4 && uci.size() != 5) || uci[0] < 'a' || uci[0] > 'h'
              || uci[1] < '1' || uci[1] > '8' || uci[2] < 'a' || uci[2] > 'h'
              || uci[3] < '1' || uci[3] > '8'
              || (uci.size() == 5 && std::string_view("qrbn").find(uci[4]) == std::string_view::npos)))
      {
        return error(400, "invalid_move", "Bad Request", "The move must be lowercase UCI notation.");
      }
      std::lock_guard<std::mutex> lock(owner.mutex());
      sqlite3* database = owner.handle();
      Transaction transaction(database);
      if (!transaction)
      {
        return internal_error();
      }
      Statement seats(database, "SELECT white_principal, black_principal, draw_offer_by, initial_ms, "
                "clock_white_ms, clock_black_ms, clock_started_at_ms "
                                "FROM games WHERE game_id = ?1");
      if (!seats || !bind_text(seats.get(), 1, game_id))
      {
        return internal_error();
      }
      const int seat_step = sqlite3_step(seats.get());
      if (seat_step == SQLITE_DONE)
      {
        return error(404, "game_not_found", "Not Found", "The requested game was not found.");
      }
      if (seat_step != SQLITE_ROW)
      {
        return internal_error();
      }
      const std::string white = column_text(seats.get(), 0);
      std::string black = column_text(seats.get(), 1);
      std::string draw_offer = column_text(seats.get(), 2);
      const bool timed = sqlite3_column_type(seats.get(), 3) != SQLITE_NULL;
        const auto stored_white_clock = timed ? sqlite3_column_int64(seats.get(), 4) : 0;
        const auto stored_black_clock = timed ? sqlite3_column_int64(seats.get(), 5) : 0;
        const auto stored_clock_started = sqlite3_column_type(seats.get(), 6) == SQLITE_INTEGER
          ? sqlite3_column_int64(seats.get(), 6) : 0;
        if (timed && (sqlite3_column_type(seats.get(), 3) != SQLITE_INTEGER
          || sqlite3_column_type(seats.get(), 4) != SQLITE_INTEGER
          || sqlite3_column_type(seats.get(), 5) != SQLITE_INTEGER
          || (sqlite3_column_type(seats.get(), 6) != SQLITE_NULL
            && sqlite3_column_type(seats.get(), 6) != SQLITE_INTEGER)
          || stored_white_clock < 0 || stored_black_clock < 0))
        return internal_error();
      if (sqlite3_step(seats.get()) != SQLITE_DONE)
      {
        return internal_error();
      }
      const bool participant = !principal.id.empty()
                               && (principal.id == white || (!black.empty() && principal.id == black));
      if (!participant && operation != Operation::join)
      {
        return error(404, "game_not_found", "Not Found", "The requested game was not found.");
      }
      const std::array<const char*, 5> names{"join", "move", "resign", "offer_draw", "accept_draw"};
      const std::string scope = std::string(names[static_cast<std::size_t>(operation)]) + ":" + game_id;
      const std::string fingerprint = Json{{"version", expected_version}, {"uci", uci}}.dump();
      Statement prior(database, "SELECT request_fingerprint, response_json FROM idempotency_records "
                                "WHERE principal_id = ?1 AND operation = ?2 AND idempotency_key = ?3 "
                                "AND created_at > strftime('%Y-%m-%dT%H:%M:%SZ', 'now', '-24 hours')");
      if (!prior || !bind_text(prior.get(), 1, principal.id) || !bind_text(prior.get(), 2, scope)
          || !bind_text(prior.get(), 3, key))
      {
        return internal_error();
      }
      const int prior_step = sqlite3_step(prior.get());
      if (prior_step != SQLITE_ROW && prior_step != SQLITE_DONE)
      {
        return internal_error();
      }
      if (prior_step == SQLITE_ROW)
      {
        if (!participant)
        {
          return error(404, "game_not_found", "Not Found", "The requested game was not found.");
        }
        if (column_text(prior.get(), 0) != fingerprint)
        {
          return error(409, "idempotency_conflict", "Conflict", "The idempotency key was already used for a different request.");
        }
        try
        {
          auto snapshot = snapshot_from_json(Json::parse(column_text(prior.get(), 1)));
          if (snapshot.game_id != game_id || sqlite3_step(prior.get()) != SQLITE_DONE || !transaction.commit())
          {
            return internal_error();
          }
          return snapshot;
        }
        catch (const ExecutionDeadlineExceeded&)
        {
          throw;
        }
        catch (...)
        {
          return internal_error();
        }
      }
      if (operation == Operation::join && (principal.id == white || !black.empty()))
      {
        return error(409, "seat_unavailable", "Conflict", "The Black seat cannot be claimed.");
      }
      Chess::ChessGameHandler handler;
      auto loaded = load_snapshot(database, Principal{operation == Operation::join ? white : principal.id}, game_id, &handler);
      if (const auto* failure = std::get_if<ApplicationError>(&loaded))
      {
        return *failure;
      }
      const auto& current = std::get<GameSnapshot>(loaded);
      const auto clock_snapshot_at = epoch_milliseconds();
      if (current.version != expected_version)
      {
        return error(412, "stale_version", "Precondition Failed", "The game version has changed.", current.version);
      }
      const auto commit_timeout = [&](const std::string& color) {
        const std::string timeout_status = color + "_timeout";
        Statement timeout(database, "UPDATE games SET status=?1,version=version+1,"
            "clock_white_ms=CASE WHEN ?2='white' THEN 0 ELSE clock_white_ms END,"
            "clock_black_ms=CASE WHEN ?2='black' THEN 0 ELSE clock_black_ms END,"
            "clock_started_at_ms=NULL,updated_at=?3 WHERE game_id=?4 AND version=?5");
        return timeout && bind_text(timeout.get(), 1, timeout_status)
            && bind_text(timeout.get(), 2, color) && bind_text(timeout.get(), 3, utc_now())
            && bind_text(timeout.get(), 4, game_id)
            && sqlite3_bind_int64(timeout.get(), 5, static_cast<sqlite3_int64>(current.version)) == SQLITE_OK
            && sqlite3_step(timeout.get()) == SQLITE_DONE && sqlite3_changes(database) == 1
            && transaction.commit();
      };
      if (timed && current.clock && current.clock->running_color)
      {
        const auto remaining = *current.clock->running_color == "white"
            ? current.clock->white_remaining_ms : current.clock->black_remaining_ms;
        if (remaining == 0)
        {
          if (!commit_timeout(*current.clock->running_color)) return internal_error();
          return error(409, "clock_expired", "Conflict", "The active game clock has expired.");
        }
      }
      if (current.version >= static_cast<std::uint64_t>((std::numeric_limits<sqlite3_int64>::max)()))
      {
        return error(409, "version_limit", "Conflict", "The game version limit has been reached.", current.version);
      }
      if (current.status != "ongoing")
      {
        return error(409, "game_finished", "Conflict", "The game is already finished.");
      }
      std::string status = current.status;
      auto white_clock = timed ? static_cast<std::int64_t>(current.clock->white_remaining_ms) : 0;
      auto black_clock = timed ? static_cast<std::int64_t>(current.clock->black_remaining_ms) : 0;
      std::optional<std::int64_t> clock_started;
      const std::string seat = principal.id == white ? "white" : "black";
      if (!draw_offer.empty() && draw_offer != "white" && draw_offer != "black")
      {
        return internal_error();
      }
      if (operation == Operation::join)
      {
        black = principal.id;
        if (timed && current.status == "ongoing") clock_started = epoch_milliseconds();
      }
      else if (operation == Operation::move)
      {
        if (black.empty() || current.side_to_move != seat)
        {
          return error(409, "wrong_turn", "Conflict", "The caller cannot move now.");
        }
        if (current.move_count >= maximum_plies)
        {
          return error(409, "history_limit", "Conflict", "The game exceeds the supported move history limit.");
        }
        if (handler.get_current_grid().flags.halfmove_clock >= (std::numeric_limits<int>::max)() - 1
            || handler.get_current_grid().flags.fullmove_number >= (std::numeric_limits<int>::max)() - 1)
        {
          return error(409, "counter_limit", "Conflict", "The position counter limit has been reached.");
        }
        Chess::ChessTranslationUnit translator;
        bool applied = false;
        std::string san;
        for (const auto& move : handler.get_legal_moves())
        {
          ExecutionBudget::checkpoint();
          if (translator.move_to_algebraic(move, handler.get_current_grid()) == uci)
          {
            san = translator.move_to_san(move, handler.get_current_grid());
            if (timed && current.clock && current.clock->running_color)
            {
              const auto elapsed = (std::max<std::int64_t>)(0, epoch_milliseconds() - clock_snapshot_at);
              auto& remaining = *current.clock->running_color == "white" ? white_clock : black_clock;
              if (elapsed >= remaining)
              {
                if (!commit_timeout(*current.clock->running_color)) return internal_error();
                return error(409, "clock_expired", "Conflict", "The active game clock has expired.");
              }
              remaining -= elapsed;
            }
            applied = handler.make_move(move);
            break;
          }
        }
        if (!applied)
        {
          return error(422, "illegal_move", "Unprocessable Content", "The move is not legal in this position.");
        }
        Statement append(database, "INSERT INTO game_moves(game_id, ply, uci, san) VALUES(?1, ?2, ?3, ?4)");
        if (!append || !bind_text(append.get(), 1, game_id)
            || sqlite3_bind_int64(append.get(), 2, static_cast<sqlite3_int64>(current.move_count + 1)) != SQLITE_OK
            || !bind_text(append.get(), 3, uci) || !bind_text(append.get(), 4, san)
            || sqlite3_step(append.get()) != SQLITE_DONE)
        {
          return internal_error();
        }
        status = status_name(handler.get_game_state());
        draw_offer.clear();
        if (timed && status == "ongoing") clock_started = epoch_milliseconds();
      }
      else if (operation == Operation::resign)
      {
        status = seat + "_resigned";
        draw_offer.clear();
      }
      else if (operation == Operation::offer_draw)
      {
        if (black.empty() || !draw_offer.empty())
        {
          return error(409, "draw_offer_unavailable", "Conflict", "A draw cannot be offered now.");
        }
        draw_offer = seat;
        if (timed && current.clock && current.clock->running_color)
        {
          white_clock = stored_white_clock;
          black_clock = stored_black_clock;
          clock_started = stored_clock_started;
        }
      }
      else
      {
        if (black.empty() || draw_offer.empty() || draw_offer == seat)
        {
          return error(409, "no_opponent_offer", "Conflict", "There is no opponent draw offer to accept.");
        }
        status = "draw_by_agreement";
        draw_offer.clear();
      }
        Statement update(database, "UPDATE games SET black_principal=?1,status=?2,draw_offer_by=?3,"
          "clock_white_ms=?4,clock_black_ms=?5,clock_started_at_ms=?6,"
          "version=version+1,updated_at=?7 WHERE game_id=?8 AND version=?9");
      if (!update || (black.empty() ? sqlite3_bind_null(update.get(), 1) != SQLITE_OK : !bind_text(update.get(), 1, black))
          || !bind_text(update.get(), 2, status)
          || (draw_offer.empty() ? sqlite3_bind_null(update.get(), 3) != SQLITE_OK : !bind_text(update.get(), 3, draw_offer))
          || (timed ? sqlite3_bind_int64(update.get(), 4, white_clock) != SQLITE_OK
              : sqlite3_bind_null(update.get(), 4) != SQLITE_OK)
          || (timed ? sqlite3_bind_int64(update.get(), 5, black_clock) != SQLITE_OK
              : sqlite3_bind_null(update.get(), 5) != SQLITE_OK)
          || (clock_started ? sqlite3_bind_int64(update.get(), 6, *clock_started) != SQLITE_OK
                  : sqlite3_bind_null(update.get(), 6) != SQLITE_OK)
          || !bind_text(update.get(), 7, utc_now()) || !bind_text(update.get(), 8, game_id)
          || sqlite3_bind_int64(update.get(), 9, static_cast<sqlite3_int64>(expected_version)) != SQLITE_OK
          || sqlite3_step(update.get()) != SQLITE_DONE || sqlite3_changes(database) != 1)
      {
        return internal_error();
      }
      auto result = load_snapshot(database, principal, game_id);
      if (const auto* failure = std::get_if<ApplicationError>(&result))
      {
        return *failure;
      }
      Statement cleanup(database, "DELETE FROM idempotency_records WHERE rowid IN "
                                  "(SELECT rowid FROM idempotency_records WHERE created_at <= "
                                  "strftime('%Y-%m-%dT%H:%M:%SZ', 'now', '-24 hours') ORDER BY created_at LIMIT 64)");
      Statement expired(database, "DELETE FROM idempotency_records WHERE principal_id = ?1 AND operation = ?2 "
                                  "AND idempotency_key = ?3 AND created_at <= "
                                  "strftime('%Y-%m-%dT%H:%M:%SZ', 'now', '-24 hours')");
      if (!cleanup || sqlite3_step(cleanup.get()) != SQLITE_DONE || !expired
          || !bind_text(expired.get(), 1, principal.id) || !bind_text(expired.get(), 2, scope)
          || !bind_text(expired.get(), 3, key) || sqlite3_step(expired.get()) != SQLITE_DONE)
      {
        return internal_error();
      }
      Statement record(database, "INSERT INTO idempotency_records(principal_id, operation, idempotency_key, "
                                 "request_fingerprint, response_json, created_at) VALUES(?1, ?2, ?3, ?4, ?5, ?6)");
      if (!record || !bind_text(record.get(), 1, principal.id) || !bind_text(record.get(), 2, scope)
          || !bind_text(record.get(), 3, key) || !bind_text(record.get(), 4, fingerprint)
          || !bind_text(record.get(), 5, snapshot_json(std::get<GameSnapshot>(result)).dump())
          || !bind_text(record.get(), 6, utc_now()) || sqlite3_step(record.get()) != SQLITE_DONE || !transaction.commit())
      {
        return internal_error();
      }
      return result;
    }
  } // namespace

  SqliteGameApplication::SqliteGameApplication(SqliteDatabase& database) : database_(database) {}

  bool SqliteGameApplication::is_ready() const
  {
    ExecutionBudget::checkpoint();
    return database_.is_ready();
  }

  bool SqliteGameApplication::cleanup_expired_idempotency(std::size_t maximum_records) noexcept
  {
    try
    {
      if (maximum_records == 0 || maximum_records > 1024 || !database_.is_ready())
      {
        maintenance_failures_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }
      std::lock_guard<std::mutex> lock(database_.mutex());
      sqlite3* database = database_.handle();
      Transaction transaction(database);
      if (!transaction)
      {
        maintenance_failures_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }
      Statement cleanup(database, "DELETE FROM idempotency_records WHERE rowid IN "
                                  "(SELECT rowid FROM idempotency_records WHERE created_at <= "
                                  "strftime('%Y-%m-%dT%H:%M:%SZ', 'now', '-24 hours') "
                                  "ORDER BY created_at LIMIT ?1)");
      if (!cleanup || sqlite3_bind_int64(cleanup.get(), 1, static_cast<sqlite3_int64>(maximum_records)) != SQLITE_OK
          || sqlite3_step(cleanup.get()) != SQLITE_DONE)
      {
        maintenance_failures_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }
      const auto deleted = static_cast<std::uint64_t>(sqlite3_changes(database));
      ExecutionBudget::checkpoint();
      if (!transaction.commit())
      {
        maintenance_failures_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }
      maintenance_deleted_records_.fetch_add(deleted, std::memory_order_relaxed);
      return true;
    }
    catch (...)
    {
      maintenance_failures_.fetch_add(1, std::memory_order_relaxed);
      return false;
    }
  }

  std::uint64_t SqliteGameApplication::maintenance_failures() const noexcept
  {
    return maintenance_failures_.load(std::memory_order_relaxed);
  }

  std::uint64_t SqliteGameApplication::maintenance_deleted_records() const noexcept
  {
    return maintenance_deleted_records_.load(std::memory_order_relaxed);
  }

  bool SqliteGameApplication::expire_timed_games(std::size_t maximum_games, std::size_t& expired) noexcept
  {
    expired = 0;
    try
    {
      if (maximum_games == 0 || maximum_games > 256 || !database_.is_ready()) return false;
      std::lock_guard<std::mutex> lock(database_.mutex());
      sqlite3* database = database_.handle();
      Transaction transaction(database);
      if (!transaction) return false;
      struct ActiveClock
      {
          std::string game_id;
          std::int64_t white_ms;
          std::int64_t black_ms;
          std::int64_t started_at_ms;
          std::int64_t version;
          std::uint64_t plies;
          bool initial_white_to_move;
      };
      std::vector<ActiveClock> active_games;
      {
        Statement query(database, "SELECT g.game_id,g.initial_fen,g.clock_white_ms,g.clock_black_ms,"
            "g.clock_started_at_ms,g.version,COUNT(m.ply) FROM games g "
            "LEFT JOIN game_moves m ON m.game_id=g.game_id "
            "WHERE g.status='ongoing' AND g.clock_started_at_ms IS NOT NULL "
            "GROUP BY g.game_id ORDER BY g.clock_started_at_ms LIMIT ?1");
        if (!query || sqlite3_bind_int64(query.get(), 1, static_cast<sqlite3_int64>(maximum_games)) != SQLITE_OK)
          return false;
        int step = SQLITE_OK;
        while ((step = sqlite3_step(query.get())) == SQLITE_ROW)
        {
          if (sqlite3_column_type(query.get(), 0) != SQLITE_TEXT
              || sqlite3_column_type(query.get(), 1) != SQLITE_TEXT
              || sqlite3_column_type(query.get(), 2) != SQLITE_INTEGER
              || sqlite3_column_type(query.get(), 3) != SQLITE_INTEGER
              || sqlite3_column_type(query.get(), 4) != SQLITE_INTEGER
              || sqlite3_column_type(query.get(), 5) != SQLITE_INTEGER
              || sqlite3_column_type(query.get(), 6) != SQLITE_INTEGER) return false;
          std::istringstream fen(column_text(query.get(), 1));
          std::string placement;
          std::string active_side;
          if (!(fen >> placement >> active_side) || (active_side != "w" && active_side != "b")) return false;
          const auto plies = sqlite3_column_int64(query.get(), 6);
          if (plies < 0 || plies > static_cast<sqlite3_int64>(maximum_plies)) return false;
          active_games.push_back(ActiveClock{column_text(query.get(), 0),
              sqlite3_column_int64(query.get(), 2), sqlite3_column_int64(query.get(), 3),
              sqlite3_column_int64(query.get(), 4), sqlite3_column_int64(query.get(), 5),
              static_cast<std::uint64_t>(plies), active_side == "w"});
        }
        if (step != SQLITE_DONE) return false;
      }
      const auto now = epoch_milliseconds();
      for (const auto& game : active_games)
      {
        const bool white_to_move = (game.plies % 2 == 0)
            ? game.initial_white_to_move : !game.initial_white_to_move;
        const auto remaining = white_to_move ? game.white_ms : game.black_ms;
        if (remaining <= 0 || now - game.started_at_ms < remaining) continue;
        const std::string side = white_to_move ? "white" : "black";
        const std::string status = side + "_timeout";
        Statement update(database, "UPDATE games SET status=?1,version=version+1,"
            "clock_white_ms=CASE WHEN ?2='white' THEN 0 ELSE clock_white_ms END,"
            "clock_black_ms=CASE WHEN ?2='black' THEN 0 ELSE clock_black_ms END,"
            "clock_started_at_ms=NULL,updated_at=?3 WHERE game_id=?4 AND version=?5 "
            "AND status='ongoing' AND clock_started_at_ms=?6");
        if (!update || !bind_text(update.get(), 1, status) || !bind_text(update.get(), 2, side)
            || !bind_text(update.get(), 3, utc_now()) || !bind_text(update.get(), 4, game.game_id)
            || sqlite3_bind_int64(update.get(), 5, game.version) != SQLITE_OK
            || sqlite3_bind_int64(update.get(), 6, game.started_at_ms) != SQLITE_OK
            || sqlite3_step(update.get()) != SQLITE_DONE) return false;
        if (sqlite3_changes(database) == 1) ++expired;
      }
      return transaction.commit();
    }
    catch (...) { return false; }
  }

  bool SqliteGameApplication::anonymize_principal(const Principal& principal) noexcept
  {
    try
    {
      if (principal.id.empty() || principal.id.size() > 128 || !database_.is_ready()) return false;
      const auto generated = random_game_id();
      if (!generated) return false;
      const Principal tombstone{"deleted-" + *generated};
      std::lock_guard<std::mutex> lock(database_.mutex());
      sqlite3* database = database_.handle();
      Transaction transaction(database);
      if (!transaction) return false;
      Statement games(database, "UPDATE games SET "
          "white_principal=CASE WHEN white_principal=?1 THEN ?2 ELSE white_principal END, "
          "black_principal=CASE WHEN black_principal=?1 THEN ?2 ELSE black_principal END "
          "WHERE white_principal=?1 OR black_principal=?1");
      if (!games || !bind_text(games.get(), 1, principal.id) || !bind_text(games.get(), 2, tombstone.id)
          || sqlite3_step(games.get()) != SQLITE_DONE) return false;
      Statement retries(database, "DELETE FROM idempotency_records WHERE principal_id=?1");
      if (!retries || !bind_text(retries.get(), 1, principal.id) || sqlite3_step(retries.get()) != SQLITE_DONE)
        return false;
      return transaction.commit();
    }
    catch (...) { return false; }
  }

  Result<GameSnapshot> SqliteGameApplication::create_game(const Principal& principal, const CreateGameCommand& command,
                                                           const std::string& idempotency_key)
  {
    ExecutionBudget::checkpoint();
    if (!database_.is_ready())
    {
      return storage_unavailable();
    }
    if (principal.id.empty() || principal.id.size() > 256 || idempotency_key.size() < 16 || idempotency_key.size() > 128)
    {
      return error(400, "invalid_request", "Bad Request", "The request could not be completed.");
    }
    if (command.time_control.has_value()
        && (command.time_control->initial_ms == 0 || command.time_control->initial_ms > 86400000
            || command.time_control->increment_ms != 0))
    {
      return error(422, "invalid_time_control", "Unprocessable Content", "Only simple initial-time clocks are supported.");
    }

    const std::string fen = command.initial_fen.value_or("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    if (fen.empty() || fen.size() > 128 || fen.find_first_not_of("prnbqkPRNBQK12345678/ wbKQkq-abcdefgh0123456789") != std::string::npos)
    {
      return error(422, "invalid_fen", "Unprocessable Content", "The initial position is invalid.");
    }
    Chess::ChessGameHandler handler;
    try
    {
      handler.load_game_from_fen(fen);
        if (handler.get_current_fen() != fen || !valid_initial_position(handler.get_current_grid())
          || !consistent_fen(fen, handler.get_current_grid()))
      {
        return error(422, "invalid_fen", "Unprocessable Content", "The initial position is invalid.");
      }
    }
    catch (const ExecutionDeadlineExceeded&)
    {
      throw;
    }
    catch (...)
    {
      return error(422, "invalid_fen", "Unprocessable Content", "The initial position is invalid.");
    }

    std::lock_guard<std::mutex> lock(database_.mutex());
    sqlite3* database = database_.handle();
    Transaction transaction(database);
    if (!transaction)
    {
      return internal_error();
    }

    Statement cleanup(database,
                       "DELETE FROM idempotency_records WHERE rowid IN "
                       "(SELECT rowid FROM idempotency_records "
                       "WHERE created_at <= strftime('%Y-%m-%dT%H:%M:%SZ', 'now', '-24 hours') "
                       "ORDER BY created_at LIMIT 64)");
    if (!cleanup || sqlite3_step(cleanup.get()) != SQLITE_DONE)
    {
      return internal_error();
    }

    const std::string fingerprint = create_fingerprint(fen, command.time_control);
    Statement prior(database,
                    "SELECT request_fingerprint, response_json FROM idempotency_records "
                    "WHERE principal_id = ?1 AND operation = 'create_game' AND idempotency_key = ?2 "
                    "AND created_at > strftime('%Y-%m-%dT%H:%M:%SZ', 'now', '-24 hours')");
    if (!prior || !bind_text(prior.get(), 1, principal.id) || !bind_text(prior.get(), 2, idempotency_key))
    {
      return internal_error();
    }
    const int prior_step = sqlite3_step(prior.get());
    if (prior_step != SQLITE_ROW && prior_step != SQLITE_DONE)
    {
      return internal_error();
    }
    if (prior_step == SQLITE_ROW)
    {
      const auto prior_fingerprint = column_text(prior.get(), 0);
      const auto prior_response = column_text(prior.get(), 1);
      if (sqlite3_step(prior.get()) != SQLITE_DONE)
      {
        return internal_error();
      }
      if (prior_fingerprint != fingerprint)
      {
        return error(409, "idempotency_conflict", "Conflict", "The idempotency key was already used for a different request.");
      }
      try
      {
        GameSnapshot snapshot = snapshot_from_json(Json::parse(prior_response));
        const auto authorized = load_snapshot(database, principal, snapshot.game_id);
        if (std::holds_alternative<ApplicationError>(authorized))
        {
          return std::get<ApplicationError>(authorized);
        }
        if (!transaction.commit())
        {
          return internal_error();
        }
        return snapshot;
      }
      catch (const ExecutionDeadlineExceeded&)
      {
        throw;
      }
      catch (...)
      {
        return internal_error();
      }
    }

    Statement expired_key(database,
                           "DELETE FROM idempotency_records "
                           "WHERE principal_id = ?1 AND operation = 'create_game' AND idempotency_key = ?2 "
                           "AND created_at <= strftime('%Y-%m-%dT%H:%M:%SZ', 'now', '-24 hours')");
    if (!expired_key || !bind_text(expired_key.get(), 1, principal.id)
        || !bind_text(expired_key.get(), 2, idempotency_key) || sqlite3_step(expired_key.get()) != SQLITE_DONE)
    {
      return internal_error();
    }

    const auto generated_id = random_game_id();
    if (!generated_id)
    {
      return internal_error();
    }
    Statement insert(database,
             "INSERT INTO games(game_id,white_principal,initial_fen,initial_ms,increment_ms,"
             "clock_white_ms,clock_black_ms,version,status,created_at,updated_at) "
             "VALUES(?1,?2,?3,?4,0,?5,?5,1,?6,?7,?7) RETURNING game_id");
    if (!insert || !bind_text(insert.get(), 1, *generated_id) || !bind_text(insert.get(), 2, principal.id)
      || !bind_text(insert.get(), 3, fen)
      || (command.time_control
        ? sqlite3_bind_int64(insert.get(), 4, static_cast<sqlite3_int64>(command.time_control->initial_ms)) != SQLITE_OK
        : sqlite3_bind_null(insert.get(), 4) != SQLITE_OK)
      || (command.time_control
        ? sqlite3_bind_int64(insert.get(), 5, static_cast<sqlite3_int64>(command.time_control->initial_ms)) != SQLITE_OK
        : sqlite3_bind_null(insert.get(), 5) != SQLITE_OK)
      || !bind_text(insert.get(), 6, initial_status(handler.get_current_grid()))
      || !bind_text(insert.get(), 7, utc_now())
        || sqlite3_step(insert.get()) != SQLITE_ROW)
    {
      return internal_error();
    }
    const std::string game_id = column_text(insert.get(), 0);
    if (sqlite3_step(insert.get()) != SQLITE_DONE)
    {
      return internal_error();
    }
    auto result = load_snapshot(database, principal, game_id);
    if (std::holds_alternative<ApplicationError>(result))
    {
      return std::get<ApplicationError>(std::move(result));
    }
    const GameSnapshot snapshot = std::get<GameSnapshot>(std::move(result));
    Statement record(database,
                     "INSERT INTO idempotency_records(principal_id, operation, idempotency_key, request_fingerprint, "
                     "response_json, created_at) VALUES(?1, 'create_game', ?2, ?3, ?4, ?5)");
    if (!record || !bind_text(record.get(), 1, principal.id) || !bind_text(record.get(), 2, idempotency_key)
        || !bind_text(record.get(), 3, fingerprint) || !bind_text(record.get(), 4, snapshot_json(snapshot).dump())
        || !bind_text(record.get(), 5, utc_now()) || sqlite3_step(record.get()) != SQLITE_DONE || !transaction.commit())
    {
      return internal_error();
    }
    return snapshot;
  }

  Result<GameSnapshot> SqliteGameApplication::get_game(const Principal& principal, const std::string& game_id) const
  {
    ExecutionBudget::checkpoint();
    if (!database_.is_ready())
    {
      return storage_unavailable();
    }
    std::lock_guard<std::mutex> lock(database_.mutex());
    Transaction transaction(database_.handle(), false);
    if (!transaction)
    {
      return storage_unavailable();
    }
    auto result = load_snapshot(database_.handle(), principal, game_id);
    if (!transaction.commit())
    {
      return internal_error();
    }
    return result;
  }

  Result<GameSnapshot> SqliteGameApplication::join_game(const Principal& principal, const std::string& game_id,
                                                        std::uint64_t version, const std::string& key)
  {
    ExecutionBudget::checkpoint();
    return mutate(database_, principal, game_id, version, key, Operation::join);
  }

  Result<LegalMoveList> SqliteGameApplication::get_legal_moves(const Principal& principal, const std::string& game_id) const
  {
    ExecutionBudget::checkpoint();
    auto result = get_game(principal, game_id);
    if (const auto* failure = std::get_if<ApplicationError>(&result))
    {
      return *failure;
    }
    auto& snapshot = std::get<GameSnapshot>(result);
    return LegalMoveList{snapshot.version, std::move(snapshot.legal_moves)};
  }

  Result<MovePage> SqliteGameApplication::get_move_history(const Principal& principal, const std::string& game_id,
                                                          std::uint64_t after_ply, std::uint64_t limit) const
  {
    ExecutionBudget::checkpoint();
    if (!database_.is_ready())
    {
      return storage_unavailable();
    }
    if (limit == 0 || limit > 200 || after_ply > maximum_plies)
    {
      return error(400, "invalid_pagination", "Bad Request", "The history cursor or page size is invalid.");
    }
    std::lock_guard<std::mutex> lock(database_.mutex());
    sqlite3* database = database_.handle();
    Transaction transaction(database, false);
    if (!transaction)
    {
      return internal_error();
    }
    const auto authorized = load_snapshot(database, principal, game_id);
    if (const auto* failure = std::get_if<ApplicationError>(&authorized))
    {
      return *failure;
    }
    Statement query(database, "SELECT ply, uci, san FROM game_moves WHERE game_id = ?1 AND ply > ?2 ORDER BY ply LIMIT ?3");
    if (!query || !bind_text(query.get(), 1, game_id)
        || sqlite3_bind_int64(query.get(), 2, static_cast<sqlite3_int64>(after_ply)) != SQLITE_OK
        || sqlite3_bind_int64(query.get(), 3, static_cast<sqlite3_int64>(limit + 1)) != SQLITE_OK)
    {
      return internal_error();
    }
    MovePage page;
    int step = SQLITE_OK;
    while ((step = sqlite3_step(query.get())) == SQLITE_ROW)
    {
      ExecutionBudget::checkpoint();
      if (page.moves.size() == limit)
      {
        page.next_after_ply = page.moves.back().ply;
        continue;
      }
      page.moves.push_back(MoveRecord{static_cast<std::uint64_t>(sqlite3_column_int64(query.get(), 0)),
                                      column_text(query.get(), 1), column_text(query.get(), 2)});
    }
    if (step != SQLITE_DONE || !transaction.commit())
    {
      return internal_error();
    }
    return page;
  }

  Result<GameSnapshot> SqliteGameApplication::submit_move(const Principal& principal, const std::string& game_id,
                                                          const MoveCommand& command, const std::string& key)
  {
    ExecutionBudget::checkpoint();
    return mutate(database_, principal, game_id, command.expected_version, key, Operation::move, command.uci);
  }

  Result<GameSnapshot> SqliteGameApplication::resign(const Principal& principal, const std::string& game_id,
                                                    std::uint64_t version, const std::string& key)
  {
    ExecutionBudget::checkpoint();
    return mutate(database_, principal, game_id, version, key, Operation::resign);
  }

  Result<GameSnapshot> SqliteGameApplication::offer_draw(const Principal& principal, const std::string& game_id,
                                                        std::uint64_t version, const std::string& key)
  {
    ExecutionBudget::checkpoint();
    return mutate(database_, principal, game_id, version, key, Operation::offer_draw);
  }

  Result<GameSnapshot> SqliteGameApplication::accept_draw(const Principal& principal, const std::string& game_id,
                                                         std::uint64_t version, const std::string& key)
  {
    ExecutionBudget::checkpoint();
    return mutate(database_, principal, game_id, version, key, Operation::accept_draw);
  }
} // namespace Chess::Backend