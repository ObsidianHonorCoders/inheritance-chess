#ifndef ICHESS_BACKEND_LOCAL_ADAPTERS
#define ICHESS_BACKEND_LOCAL_ADAPTERS

#include "game_application.h"
#include "request_authenticator.h"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Chess::Backend
{
  struct IssuedGuestSession
  {
      std::string cookie_value;
      std::string csrf_token;
  };

  class LocalSessionAuthenticator final : public IRequestAuthenticator
  {
    public:
      explicit LocalSessionAuthenticator(std::vector<std::string> allowed_origins);

      IssuedGuestSession issue_guest_session(const std::string& display_name);
      bool allows_origin(const std::string& origin) const;
      AuthenticationResult authenticate(const httplib::Request& request, bool state_changing) const override;

    private:
      struct Session
      {
          std::string principal_id;
          std::string display_name;
          std::string csrf_token;
      };

      std::vector<std::string> allowed_origins_;
      mutable std::mutex mutex_;
      std::unordered_map<std::string, Session> sessions_;
  };

  void register_local_session_routes(httplib::Server& server, LocalSessionAuthenticator& authenticator);

  class InMemoryGameApplication final : public IGameApplication
  {
    public:
      InMemoryGameApplication();
      ~InMemoryGameApplication() override;

      bool is_ready() const override;
      Result<GameSnapshot> create_game(const Principal& principal,
                                       const CreateGameCommand& command,
                                       const std::string& idempotency_key) override;
      Result<GameSnapshot> get_game(const Principal& principal, const std::string& game_id) const override;
      Result<GameSnapshot> join_game(const Principal& principal,
                                     const std::string& game_id,
                                     std::uint64_t expected_version,
                                     const std::string& idempotency_key) override;
      Result<LegalMoveList> get_legal_moves(const Principal& principal, const std::string& game_id) const override;
      Result<MovePage> get_move_history(const Principal& principal,
                                        const std::string& game_id,
                                        std::uint64_t after_ply,
                                        std::uint64_t limit) const override;
      Result<GameSnapshot> submit_move(const Principal& principal,
                                       const std::string& game_id,
                                       const MoveCommand& command,
                                       const std::string& idempotency_key) override;
      Result<GameSnapshot> resign(const Principal& principal,
                                  const std::string& game_id,
                                  std::uint64_t expected_version,
                                  const std::string& idempotency_key) override;
      Result<GameSnapshot> offer_draw(const Principal& principal,
                                      const std::string& game_id,
                                      std::uint64_t expected_version,
                                      const std::string& idempotency_key) override;
      Result<GameSnapshot> accept_draw(const Principal& principal,
                                       const std::string& game_id,
                                       std::uint64_t expected_version,
                                       const std::string& idempotency_key) override;

    private:
      struct GameRecord;
      struct IdempotencyEntry
      {
          std::string fingerprint;
          GameSnapshot snapshot;
      };

      std::string idempotency_map_key(const Principal& principal,
                                      const std::string& operation,
                                      const std::string& key) const;
      Result<GameSnapshot> cached_result(const Principal& principal,
                                         const std::string& operation,
                                         const std::string& key,
                                         const std::string& fingerprint) const;
      void remember_result(const Principal& principal,
                           const std::string& operation,
                           const std::string& key,
                           const std::string& fingerprint,
                           const GameSnapshot& snapshot);
      GameSnapshot make_snapshot(const GameRecord& game, const Principal& viewer) const;
      GameRecord* authorized_game(const Principal& principal, const std::string& game_id);
      const GameRecord* authorized_game(const Principal& principal, const std::string& game_id) const;

      mutable std::mutex mutex_;
      std::unordered_map<std::string, std::unique_ptr<GameRecord>> games_;
      std::unordered_map<std::string, IdempotencyEntry> idempotency_;
  };
} // namespace Chess::Backend

#endif // ICHESS_BACKEND_LOCAL_ADAPTERS
