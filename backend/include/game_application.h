#ifndef ICHESS_BACKEND_GAME_APPLICATION
#define ICHESS_BACKEND_GAME_APPLICATION

#include "api_types.h"
#include <cstdint>
#include <string>

namespace Chess::Backend
{
  class IGameApplication
  {
    public:
      virtual ~IGameApplication() = default;

      virtual bool is_ready() const = 0;
      virtual Result<GameSnapshot> create_game(const Principal& principal,
                                               const CreateGameCommand& command,
                                               const std::string& idempotency_key) = 0;
      virtual Result<GameSnapshot> get_game(const Principal& principal,
                                             const std::string& game_id) const = 0;
      virtual Result<GameSnapshot> join_game(const Principal& principal,
                                              const std::string& game_id,
                                              std::uint64_t expected_version,
                                              const std::string& idempotency_key) = 0;
      virtual Result<LegalMoveList> get_legal_moves(const Principal& principal,
                                                    const std::string& game_id) const = 0;
      virtual Result<MovePage> get_move_history(const Principal& principal,
                                                const std::string& game_id,
                                                std::uint64_t after_ply,
                                                std::uint64_t limit) const = 0;
      virtual Result<GameSnapshot> submit_move(const Principal& principal,
                                               const std::string& game_id,
                                               const MoveCommand& command,
                                               const std::string& idempotency_key) = 0;
      virtual Result<GameSnapshot> resign(const Principal& principal,
                                          const std::string& game_id,
                                          std::uint64_t expected_version,
                                          const std::string& idempotency_key) = 0;
      virtual Result<GameSnapshot> offer_draw(const Principal& principal,
                                              const std::string& game_id,
                                              std::uint64_t expected_version,
                                              const std::string& idempotency_key) = 0;
      virtual Result<GameSnapshot> accept_draw(const Principal& principal,
                                               const std::string& game_id,
                                               std::uint64_t expected_version,
                                               const std::string& idempotency_key) = 0;
  };
} // namespace Chess::Backend

#endif // ICHESS_BACKEND_GAME_APPLICATION
