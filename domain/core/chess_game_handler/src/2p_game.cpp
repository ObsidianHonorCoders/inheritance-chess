/// @file      2p_game.cpp
/// @namespace Chess
/// @brief     Two-player console chess game for Inheritance Chess.
/// @author    Calileus
/// @date      2026-05-07
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Asks each player for their move, accepts SAN or long algebraic
///            notation, validates through the full CGH/CBM pipeline, and
///            re-prompts clearly on illegal input.  No AI involvement.

#include <algorithm>
#include <cctype>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "game_handler.h"
#include "translation_unit.h"

namespace
{
  std::string trim_copy(const std::string& s)
  {
    const auto first = s.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
    {
      return "";
    }
    const auto last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, last - first + 1);
  }

  std::string to_lower_copy(std::string s)
  {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    return s;
  }

  std::string strip_san_suffixes(std::string s)
  {
    while (!s.empty())
    {
      const char c = s.back();
      if (c == '+' || c == '#' || c == '!' || c == '?')
      {
        s.pop_back();
      }
      else
      {
        break;
      }
    }
    return s;
  }

  std::string color_to_string(Chess::Color c)
  {
    return (c == Chess::Color::WHITE) ? "White" : "Black";
  }

  std::string game_state_label(Chess::GameState s)
  {
    switch (s)
    {
    case Chess::GameState::CHECKMATE:
      return "Checkmate";
    case Chess::GameState::STALEMATE:
      return "Stalemate — draw";
    case Chess::GameState::DRAW_INSUFFICIENT_MATERIAL:
      return "Draw — insufficient material";
    case Chess::GameState::DRAW_FIFTY_MOVE_RULE:
      return "Draw — fifty-move rule";
    case Chess::GameState::DRAW_THREEFOLD_REPETITION:
      return "Draw — threefold repetition";
    case Chess::GameState::WHITE_RESIGNED:
      return "White resigned";
    case Chess::GameState::BLACK_RESIGNED:
      return "Black resigned";
    case Chess::GameState::DRAW_BY_AGREEMENT:
      return "Draw by agreement";
    case Chess::GameState::WHITE_TIMEOUT:
      return "White lost on time";
    case Chess::GameState::BLACK_TIMEOUT:
      return "Black lost on time";
    default:
      return "Game over";
    }
  }

  // Try to resolve the user string to a legal Move.
  // Accepts long algebraic (e2e4) and SAN (Nf3, e4, O-O, e8=Q).
  std::optional<Chess::Move> resolve_move(const std::string&                 input,
                                          const Chess::ChessGameHandler&      handler,
                                          const Chess::ChessTranslationUnit&   tu)
  {
    const std::string trimmed = trim_copy(input);
    if (trimmed.empty())
    {
      return std::nullopt;
    }

    const std::string lowered = to_lower_copy(trimmed);
    const std::string stripped_san = strip_san_suffixes(trimmed);
    const std::string lowered_stripped = to_lower_copy(stripped_san);

    const Chess::Grid& grid = handler.get_current_grid();
    const auto legal_moves = handler.get_legal_moves();

    for (const auto& legal : legal_moves)
    {
      // Long algebraic match (case-insensitive)
      const std::string long_alg = to_lower_copy(tu.move_to_algebraic(legal, grid));
      if (!long_alg.empty() && long_alg == lowered)
      {
        return legal;
      }

      // SAN match (case-sensitive preferred, then case-insensitive)
      const std::string san = tu.move_to_san(legal, grid);
      if (san == trimmed || strip_san_suffixes(san) == stripped_san
          || to_lower_copy(san) == lowered || to_lower_copy(strip_san_suffixes(san)) == lowered_stripped)
      {
        return legal;
      }
    }

    return std::nullopt;
  }

  void print_commands()
  {
    std::cout << "  <move>         Play a move (SAN: Nf3, e4, O-O, e8=Q  or  long: e2e4, g1f3)\n";
    std::cout << "  moves          List all legal moves\n";
    std::cout << "  board          Redraw the board\n";
    std::cout << "  resign         Current player forfeits\n";
    std::cout << "  draw           Offer/accept immediate draw by agreement\n";
    std::cout << "  fen            Print current FEN\n";
    std::cout << "  help / ?       Show this list\n";
  }
} // namespace

int main()
{
  Chess::ChessGameHandler game;
  Chess::ChessTranslationUnit tu;

  std::cout << "============================================================\n";
  std::cout << " Inheritance Chess — Two-Player Console Game\n";
  std::cout << "============================================================\n\n";

  // Ask for player names
  std::string white_name;
  std::string black_name;

  std::cout << "Player 1 name (White): ";
  if (!std::getline(std::cin, white_name) || trim_copy(white_name).empty())
  {
    white_name = "Player 1";
  }
  white_name = trim_copy(white_name);

  std::cout << "Player 2 name (Black): ";
  if (!std::getline(std::cin, black_name) || trim_copy(black_name).empty())
  {
    black_name = "Player 2";
  }
  black_name = trim_copy(black_name);

  std::cout << "\nWelcome, " << white_name << " (White) and " << black_name << " (Black)!\n";
  std::cout << "Type 'help' for available commands.\n\n";

  game.start_new_game();
  game.display_board();

  while (game.get_game_state() == Chess::GameState::ONGOING)
  {
    const Chess::Color turn = game.get_current_grid().current_turn;
    const std::string& current_name = (turn == Chess::Color::WHITE) ? white_name : black_name;
    const std::string& waiting_name = (turn == Chess::Color::WHITE) ? black_name : white_name;
    (void)waiting_name;

    std::cout << "\n" << current_name << " (" << color_to_string(turn) << ") > ";

    std::string line;
    if (!std::getline(std::cin, line))
    {
      std::cout << "\nInput closed. Exiting.\n";
      break;
    }

    line = trim_copy(line);
    if (line.empty())
    {
      continue;
    }

    const std::string lower_line = to_lower_copy(line);

    if (lower_line == "help" || lower_line == "?")
    {
      print_commands();
      continue;
    }

    if (lower_line == "board")
    {
      game.display_board();
      continue;
    }

    if (lower_line == "fen")
    {
      std::cout << game.get_current_fen() << "\n";
      continue;
    }

    if (lower_line == "moves")
    {
      const Chess::Grid& grid = game.get_current_grid();
      const auto legal_moves = game.get_legal_moves();
      std::cout << "Legal moves (" << legal_moves.size() << "):\n  ";
      int count = 0;
      for (const auto& m : legal_moves)
      {
        const std::string san = tu.move_to_san(m, grid);
        std::cout << san;
        if (++count < static_cast<int>(legal_moves.size()))
        {
          std::cout << ", ";
        }
        if (count % 10 == 0 && count < static_cast<int>(legal_moves.size()))
        {
          std::cout << "\n  ";
        }
      }
      std::cout << "\n";
      continue;
    }

    if (lower_line == "resign")
    {
      if (!game.resign(turn))
      {
        std::cout << "Unable to resign: game is already over.\n";
        continue;
      }
      game.display_board();
      break;
    }

    if (lower_line == "draw")
    {
      const std::string& their_name = (turn == Chess::Color::WHITE) ? black_name : white_name;
      const Chess::Color their_color = (turn == Chess::Color::WHITE) ? Chess::Color::BLACK : Chess::Color::WHITE;

      if (game.is_draw_offered_by(their_color))
      {
        if (!game.accept_draw(turn))
        {
          std::cout << "Unable to accept draw offer.\n";
          continue;
        }
        std::cout << current_name << " accepts the draw offer from " << their_name << ".\n";
        game.display_board();
        break;
      }
      else
      {
        if (!game.offer_draw(turn))
        {
          std::cout << "Unable to offer draw: game is already over.\n";
          continue;
        }
        std::cout << current_name << " offers a draw. " << their_name
                  << ", type 'draw' on your turn to accept.\n";
        continue;
      }
    }

    // Treat anything else as a move attempt
    const auto resolved = resolve_move(line, game, tu);
    if (!resolved.has_value())
    {
      std::cout << "  !! '" << line << "' is not a legal move.\n";
      std::cout << "     Type 'moves' to list all legal moves, or 'help' for commands.\n";
      continue;
    }

    const std::string played_san = tu.move_to_san(*resolved, game.get_current_grid());
    if (!game.make_move(*resolved))
    {
      std::cout << "  !! Move could not be applied.\n";
      continue;
    }
    std::cout << "  >> " << current_name << " plays: " << played_san << "\n";
    game.display_board();

    const Chess::GameState state = game.get_game_state();
    if (state != Chess::GameState::ONGOING)
    {
      std::cout << "------------------------------------------------------------\n";
      std::cout << game_state_label(state) << "\n";

      if (state == Chess::GameState::CHECKMATE)
      {
        // The player who just moved wins — turn has already switched in CGH
        const Chess::Color loser = game.get_current_grid().current_turn;
        const Chess::Color winner_color = (loser == Chess::Color::WHITE) ? Chess::Color::BLACK : Chess::Color::WHITE;
        const std::string& winner_n = (winner_color == Chess::Color::WHITE) ? white_name : black_name;
        std::cout << winner_n << " (" << color_to_string(winner_color) << ") wins!\n";
      }
      else if (state == Chess::GameState::WHITE_RESIGNED || state == Chess::GameState::WHITE_TIMEOUT)
      {
        std::cout << black_name << " (Black) wins!\n";
      }
      else if (state == Chess::GameState::BLACK_RESIGNED || state == Chess::GameState::BLACK_TIMEOUT)
      {
        std::cout << white_name << " (White) wins!\n";
      }

      std::cout << "------------------------------------------------------------\n";
    }
  }

  std::cout << "\nThanks for playing! — OHC team.\n";
  return 0;
}
