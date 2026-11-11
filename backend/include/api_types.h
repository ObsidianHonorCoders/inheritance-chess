#ifndef ICHESS_BACKEND_API_TYPES
#define ICHESS_BACKEND_API_TYPES

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Chess::Backend
{
  struct Principal
  {
      std::string id;
      std::string display_name;

      Principal() = default;
      explicit Principal(std::string principal_id, std::string name = {})
          : id(std::move(principal_id)), display_name(std::move(name))
      {
      }
  };

  struct ApplicationError
  {
      int         status = 500;
      std::string code = "internal_error";
      std::string title = "Internal Server Error";
      std::string detail = "The request could not be completed.";
      std::optional<std::uint64_t> current_version;
  };

  template <typename T>
  using Result = std::variant<T, ApplicationError>;

  struct TimeControl
  {
      std::uint64_t initial_ms = 600000;
      std::uint64_t increment_ms = 0;
  };

  struct CreateGameCommand
  {
      std::optional<std::string> initial_fen;
      std::optional<TimeControl> time_control;
  };

  struct LegalMove
  {
      std::string uci;
      std::string san;
  };

  struct Player
  {
      std::string display_name;
  };

  struct ClockSnapshot
  {
      std::uint64_t white_remaining_ms = 0;
      std::uint64_t black_remaining_ms = 0;
      std::optional<std::string> running_color;
      std::uint64_t increment_ms = 0;
  };

  struct GameSnapshot
  {
      std::string game_id;
      std::uint64_t version = 1;
      std::string fen;
      std::string side_to_move;
      std::string status;
      std::vector<LegalMove> legal_moves;
      std::uint64_t move_count = 0;
      std::string viewer_seat;
      Player white_player;
      std::optional<Player> black_player;
      std::optional<ClockSnapshot> clock;
      std::string server_time;
  };

  struct MoveRecord
  {
      std::uint64_t ply = 0;
      std::string uci;
      std::string san;
  };

  struct MovePage
  {
      std::vector<MoveRecord> moves;
      std::optional<std::uint64_t> next_after_ply;
  };

  struct LegalMoveList
  {
      std::uint64_t version = 1;
      std::vector<LegalMove> legal_moves;
  };

  struct MoveCommand
  {
      std::string uci;
      std::uint64_t expected_version = 0;
  };
} // namespace Chess::Backend

#endif // ICHESS_BACKEND_API_TYPES
