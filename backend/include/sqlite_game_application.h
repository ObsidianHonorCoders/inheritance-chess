#ifndef ICHESS_BACKEND_SQLITE_GAME_APPLICATION
#define ICHESS_BACKEND_SQLITE_GAME_APPLICATION

#include "game_application.h"
#include "sqlite_database.h"
#include <mutex>

namespace Chess::Backend
{
  class SqliteGameApplication final : public IGameApplication
  {
    public:
      explicit SqliteGameApplication(SqliteDatabase& database);

      bool is_ready() const override;
      Result<GameSnapshot> create_game(const Principal& principal, const CreateGameCommand& command,
                                       const std::string& idempotency_key) override;
      Result<GameSnapshot> get_game(const Principal& principal, const std::string& game_id) const override;
      Result<GameSnapshot> join_game(const Principal&, const std::string&, std::uint64_t,
                                     const std::string&) override;
      Result<LegalMoveList> get_legal_moves(const Principal&, const std::string&) const override;
      Result<MovePage> get_move_history(const Principal&, const std::string&, std::uint64_t,
                                        std::uint64_t) const override;
      Result<GameSnapshot> submit_move(const Principal&, const std::string&, const MoveCommand&,
                                       const std::string&) override;
      Result<GameSnapshot> resign(const Principal&, const std::string&, std::uint64_t,
                                  const std::string&) override;
      Result<GameSnapshot> offer_draw(const Principal&, const std::string&, std::uint64_t,
                                      const std::string&) override;
      Result<GameSnapshot> accept_draw(const Principal&, const std::string&, std::uint64_t,
                                       const std::string&) override;

    private:
      SqliteDatabase& database_;
  };
} // namespace Chess::Backend

#endif // ICHESS_BACKEND_SQLITE_GAME_APPLICATION