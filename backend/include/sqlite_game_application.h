#ifndef ICHESS_BACKEND_SQLITE_GAME_APPLICATION
#define ICHESS_BACKEND_SQLITE_GAME_APPLICATION

#include "game_application.h"
#include "sqlite_database.h"
#include <cstddef>
#include <mutex>

namespace Chess::Backend
{
  class SqliteGameApplication final : public IGameApplication
  {
    public:
      explicit SqliteGameApplication(SqliteDatabase& database);

      bool is_ready() const override;
      bool cleanup_expired_idempotency(std::size_t maximum_records = 256) noexcept;
      Result<GameSnapshot> create_game(const Principal& principal, const CreateGameCommand& command,
                                       const std::string& idempotency_key) override;
      Result<GameSnapshot> get_game(const Principal& principal, const std::string& game_id) const override;
      Result<GameSnapshot> join_game(const Principal& principal, const std::string& game_id,
                 std::uint64_t expected_version, const std::string& idempotency_key) override;
      Result<LegalMoveList> get_legal_moves(const Principal& principal, const std::string& game_id) const override;
      Result<MovePage> get_move_history(const Principal& principal, const std::string& game_id,
                    std::uint64_t after_ply, std::uint64_t limit) const override;
      Result<GameSnapshot> submit_move(const Principal& principal, const std::string& game_id,
                   const MoveCommand& command, const std::string& idempotency_key) override;
      Result<GameSnapshot> resign(const Principal& principal, const std::string& game_id,
                  std::uint64_t expected_version, const std::string& idempotency_key) override;
      Result<GameSnapshot> offer_draw(const Principal& principal, const std::string& game_id,
                  std::uint64_t expected_version, const std::string& idempotency_key) override;
      Result<GameSnapshot> accept_draw(const Principal& principal, const std::string& game_id,
                   std::uint64_t expected_version, const std::string& idempotency_key) override;

    private:
      SqliteDatabase& database_;
  };
} // namespace Chess::Backend

#endif // ICHESS_BACKEND_SQLITE_GAME_APPLICATION