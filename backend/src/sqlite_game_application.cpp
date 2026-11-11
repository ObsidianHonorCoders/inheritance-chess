#include "sqlite_game_application.h"

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
          : database_(database), active_(sqlite3_exec(database, write ? "BEGIN IMMEDIATE" : "BEGIN", nullptr, nullptr, nullptr) == SQLITE_OK) {}
        ~Transaction()
        {
          if (active_)
          {
            sqlite3_exec(database_, "ROLLBACK", nullptr, nullptr, nullptr);
          }
        }
        bool commit()
        {
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
      return value == nullptr ? std::string{} : reinterpret_cast<const char*>(value);
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

    std::string initial_status(const Chess::Grid& grid)
    {
      Chess::ChessBoardManager board;
      if (board.is_checkmate(grid, grid.current_turn))
      {
        return "checkmate";
      }
      if (board.is_stalemate(grid, grid.current_turn))
      {
        return "stalemate";
      }
      switch (board.get_draw_reason(grid, grid.current_turn))
      {
      case Chess::DrawReason::INSUFFICIENT_MATERIAL: return "draw_insufficient_material";
      case Chess::DrawReason::FIFTY_MOVE_RULE: return "draw_fifty_move_rule";
      case Chess::DrawReason::NONE: return "ongoing";
      }
      return "ongoing";
    }

    Result<GameSnapshot> load_snapshot(sqlite3* database, const Principal& principal, const std::string& game_id)
    {
      Statement game_query(database,
                           "SELECT white_principal, black_principal, initial_fen, version, status, updated_at "
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
      const std::uint64_t version = static_cast<std::uint64_t>(sqlite3_column_int64(game_query.get(), 3));
      const std::string persisted_status = column_text(game_query.get(), 4);

      Chess::ChessGameHandler handler;
      Chess::ChessTranslationUnit translator;
      try
      {
        handler.load_game_from_fen(initial_fen);
      }
      catch (...)
      {
        return internal_error();
      }

      Statement move_query(database, "SELECT uci, ply FROM game_moves WHERE game_id = ?1 ORDER BY ply");
      if (!move_query || !bind_text(move_query.get(), 1, game_id))
      {
        return internal_error();
      }
      std::uint64_t move_count = 0;
      int move_step = SQLITE_OK;
      while ((move_step = sqlite3_step(move_query.get())) == SQLITE_ROW)
      {
        if (sqlite3_column_int64(move_query.get(), 1) != static_cast<sqlite3_int64>(move_count + 1))
        {
          return internal_error();
        }
        const std::string uci = column_text(move_query.get(), 0);
        bool applied = false;
        for (const auto& move : handler.get_legal_moves())
        {
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
      if (snapshot.status == "ongoing")
      {
        for (const auto& move : handler.get_legal_moves())
        {
          snapshot.legal_moves.push_back(LegalMove{translator.move_to_algebraic(move, handler.get_current_grid()),
                                                   translator.move_to_san(move, handler.get_current_grid())});
        }
      }
      return snapshot;
    }

    Json snapshot_json(const GameSnapshot& snapshot)
    {
      Json legal_moves = Json::array();
      for (const auto& move : snapshot.legal_moves)
      {
        legal_moves.push_back(Json{{"uci", move.uci}, {"san", move.san}});
      }
      return Json{{"gameId", snapshot.game_id}, {"version", snapshot.version}, {"fen", snapshot.fen},
                  {"sideToMove", snapshot.side_to_move}, {"status", snapshot.status},
                  {"legalMoves", std::move(legal_moves)}, {"moveCount", snapshot.move_count},
                  {"viewerSeat", snapshot.viewer_seat},
                  {"players", {{"white", {{"displayName", snapshot.white_player.display_name}}},
                               {"black", snapshot.black_player ? Json{{"displayName", snapshot.black_player->display_name}} : Json(nullptr)}}},
                  {"clock", nullptr}, {"serverTime", snapshot.server_time}};
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
      for (const auto& move : value.at("legalMoves"))
      {
        snapshot.legal_moves.push_back(LegalMove{move.at("uci").get<std::string>(), move.at("san").get<std::string>()});
      }
      return snapshot;
    }

    std::string create_fingerprint(const std::string& fen)
    {
      return std::to_string(fen.size()) + ":" + fen;
    }

    template <typename T>
    Result<T> not_implemented()
    {
      return error(501, "operation_unavailable", "Not Implemented", "This game operation is not available yet.");
    }
  } // namespace

  SqliteGameApplication::SqliteGameApplication(SqliteDatabase& database) : database_(database) {}

  bool SqliteGameApplication::is_ready() const
  {
    return database_.is_ready();
  }

  Result<GameSnapshot> SqliteGameApplication::create_game(const Principal& principal, const CreateGameCommand& command,
                                                           const std::string& idempotency_key)
  {
    if (!database_.is_ready())
    {
      return storage_unavailable();
    }
    if (principal.id.empty() || principal.id.size() > 256 || idempotency_key.size() < 16 || idempotency_key.size() > 128)
    {
      return error(400, "invalid_request", "Bad Request", "The request could not be completed.");
    }
    if (command.time_control.has_value())
    {
      return error(422, "time_control_unavailable", "Unprocessable Content", "Timed games are not available yet.");
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
      if (handler.get_current_fen() != fen || !valid_initial_position(handler.get_current_grid()))
      {
        return error(422, "invalid_fen", "Unprocessable Content", "The initial position is invalid.");
      }
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

    const std::string fingerprint = create_fingerprint(fen);
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
      if (column_text(prior.get(), 0) != fingerprint)
      {
        return error(409, "idempotency_conflict", "Conflict", "The idempotency key was already used for a different request.");
      }
      try
      {
        GameSnapshot snapshot = snapshot_from_json(Json::parse(column_text(prior.get(), 1)));
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
                     "INSERT INTO games(game_id, white_principal, initial_fen, version, status, created_at, updated_at) "
                     "VALUES(?4, ?1, ?2, 1, 'ongoing', ?3, ?3) RETURNING game_id");
    if (!insert || !bind_text(insert.get(), 1, principal.id) || !bind_text(insert.get(), 2, fen)
        || !bind_text(insert.get(), 3, utc_now()) || !bind_text(insert.get(), 4, *generated_id)
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

  Result<GameSnapshot> SqliteGameApplication::join_game(const Principal&, const std::string&, std::uint64_t,
                                                         const std::string&) { return not_implemented<GameSnapshot>(); }
  Result<LegalMoveList> SqliteGameApplication::get_legal_moves(const Principal&, const std::string&) const { return not_implemented<LegalMoveList>(); }
  Result<MovePage> SqliteGameApplication::get_move_history(const Principal&, const std::string&, std::uint64_t,
                                                           std::uint64_t) const { return not_implemented<MovePage>(); }
  Result<GameSnapshot> SqliteGameApplication::submit_move(const Principal&, const std::string&, const MoveCommand&,
                                                           const std::string&) { return not_implemented<GameSnapshot>(); }
  Result<GameSnapshot> SqliteGameApplication::resign(const Principal&, const std::string&, std::uint64_t,
                                                      const std::string&) { return not_implemented<GameSnapshot>(); }
  Result<GameSnapshot> SqliteGameApplication::offer_draw(const Principal&, const std::string&, std::uint64_t,
                                                          const std::string&) { return not_implemented<GameSnapshot>(); }
  Result<GameSnapshot> SqliteGameApplication::accept_draw(const Principal&, const std::string&, std::uint64_t,
                                                           const std::string&) { return not_implemented<GameSnapshot>(); }
} // namespace Chess::Backend