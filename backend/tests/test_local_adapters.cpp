#include "local_adapters.h"

#include <gtest/gtest.h>
#include <httplib.h>
#include <variant>

namespace
{
  using namespace Chess::Backend;

  TEST(LocalSessionAuthenticatorTest, RequiresIssuedCookieCsrfAndAllowedOrigin)
  {
    LocalSessionAuthenticator authenticator({"http://localhost:5173"});
    const IssuedGuestSession session = authenticator.issue_guest_session("Guest");
    httplib::Request request;
    request.headers.emplace("Cookie", "ichess_dev_session=" + session.cookie_value);

    EXPECT_TRUE(authenticator.authenticate(request, false).authenticated);
    request.headers.emplace("Origin", "http://localhost:5173");
    request.headers.emplace("X-CSRF-Token", session.csrf_token);
    EXPECT_TRUE(authenticator.authenticate(request, true).authenticated);

    request.headers.erase("Origin");
    request.headers.emplace("Origin", "https://attacker.example");
    EXPECT_FALSE(authenticator.authenticate(request, true).authenticated);
    request.headers.erase("Origin");
    request.headers.emplace("Origin", "http://localhost:5173");
    request.headers.erase("X-CSRF-Token");
    request.headers.emplace("X-CSRF-Token", "incorrect");
    EXPECT_FALSE(authenticator.authenticate(request, true).authenticated);
  }

  TEST(InMemoryGameApplicationTest, UsesDomainLegalMovesAndEnforcesSeatsAndVersions)
  {
    InMemoryGameApplication application;
    const Principal white{"white-player"};
    const Principal black{"black-player"};
    const auto created = application.create_game(white, CreateGameCommand{}, "create-key-00000001");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(created));
    const GameSnapshot first = std::get<GameSnapshot>(created);
    EXPECT_EQ(first.viewer_seat, "white");
    EXPECT_EQ(first.version, 1u);
    ASSERT_FALSE(first.legal_moves.empty());

    const auto private_read = application.get_game(black, first.game_id);
    ASSERT_TRUE(std::holds_alternative<ApplicationError>(private_read));
    EXPECT_EQ(std::get<ApplicationError>(private_read).status, 404);

    const auto joined = application.join_game(black, first.game_id, first.version, "join-key-000000001");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(joined));
    const GameSnapshot second = std::get<GameSnapshot>(joined);
    EXPECT_EQ(second.viewer_seat, "black");
    ASSERT_EQ(second.version, 2u);

    const MoveCommand white_move{"e2e4", second.version};
    const auto moved = application.submit_move(white, first.game_id, white_move, "move-key-000000001");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(moved));
    const GameSnapshot after_white = std::get<GameSnapshot>(moved);
    EXPECT_EQ(after_white.fen, "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    EXPECT_EQ(after_white.move_count, 1u);

    const auto replay = application.submit_move(white, first.game_id, white_move, "move-key-000000001");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(replay));
    EXPECT_EQ(std::get<GameSnapshot>(replay).version, after_white.version);

    const auto wrong_seat = application.submit_move(white, first.game_id,
                                                    MoveCommand{"e7e5", after_white.version},
                                                    "move-key-000000002");
    ASSERT_TRUE(std::holds_alternative<ApplicationError>(wrong_seat));
    EXPECT_EQ(std::get<ApplicationError>(wrong_seat).status, 403);

    const auto black_move = application.submit_move(black, first.game_id,
                                                    MoveCommand{"e7e5", after_white.version},
                                                    "move-key-000000003");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(black_move));
    EXPECT_EQ(std::get<GameSnapshot>(black_move).move_count, 2u);
  }

  TEST(InMemoryGameApplicationTest, RejectsInvalidMovesAndIdempotencyKeyReuse)
  {
    InMemoryGameApplication application;
    const Principal white{"white-player"};
    const auto created = application.create_game(white, CreateGameCommand{}, "create-key-00000002");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(created));
    const GameSnapshot game = std::get<GameSnapshot>(created);
    const auto replay = application.create_game(white, CreateGameCommand{}, "create-key-00000002");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(replay));
    EXPECT_EQ(std::get<GameSnapshot>(replay).game_id, game.game_id);

    const auto joined = application.join_game(Principal{"black-player"}, game.game_id,
                                              game.version, "join-key-000000002");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(joined));
    const auto invalid_move = application.submit_move(white, game.game_id,
                                                      MoveCommand{"e2e5", std::get<GameSnapshot>(joined).version},
                                                      "move-key-000000004");
    ASSERT_TRUE(std::holds_alternative<ApplicationError>(invalid_move));
    EXPECT_EQ(std::get<ApplicationError>(invalid_move).status, 422);

    const auto first_retry_key = application.create_game(white, CreateGameCommand{}, "create-key-reuse-0001");
    ASSERT_TRUE(std::holds_alternative<GameSnapshot>(first_retry_key));
    CreateGameCommand different_position;
    different_position.initial_fen = "invalid";
    const auto reused_key = application.create_game(white, different_position, "create-key-reuse-0001");
    ASSERT_TRUE(std::holds_alternative<ApplicationError>(reused_key));
    EXPECT_EQ(std::get<ApplicationError>(reused_key).status, 409);
  }
} // namespace
