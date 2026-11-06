/// @file      ichess_runner.cpp
/// @namespace Chess
/// @brief     Entry point for Inheritance ChessEngine application.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Interactive command-line runner for playing and inspecting games
///            against the engine core modules.

#include <algorithm>
#include <cctype>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "game_handler.h"
#include "chess_engine.h"
#include "translation_unit.h"

namespace
{
  std::string trim_copy(const std::string& input)
  {
    const auto first = input.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
    {
      return "";
    }

    const auto last = input.find_last_not_of(" \t\n\r");
    return input.substr(first, last - first + 1);
  }

  std::string to_lower_copy(std::string value)
  {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    return value;
  }

  std::string strip_san_suffixes(std::string value)
  {
    while (!value.empty())
    {
      const char c = value.back();
      if (c == '+' || c == '#' || c == '!' || c == '?')
      {
        value.pop_back();
        continue;
      }
      break;
    }
    return value;
  }

  std::string color_to_string(Chess::Color color) { return (color == Chess::Color::WHITE) ? "White" : "Black"; }

  std::string game_state_to_string(Chess::GameState state)
  {
    switch (state)
    {
    case Chess::GameState::ONGOING:
      return "ongoing";
    case Chess::GameState::CHECKMATE:
      return "checkmate";
    case Chess::GameState::STALEMATE:
      return "stalemate";
    case Chess::GameState::DRAW_INSUFFICIENT_MATERIAL:
      return "draw (insufficient material)";
    case Chess::GameState::DRAW_THREEFOLD_REPETITION:
      return "draw (threefold repetition)";
    case Chess::GameState::DRAW_FIFTY_MOVE_RULE:
      return "draw (fifty-move rule)";
    case Chess::GameState::WHITE_RESIGNED:
      return "white resigned";
    case Chess::GameState::BLACK_RESIGNED:
      return "black resigned";
    case Chess::GameState::DRAW_BY_AGREEMENT:
      return "draw (agreement)";
    case Chess::GameState::WHITE_TIMEOUT:
      return "white timeout";
    case Chess::GameState::BLACK_TIMEOUT:
      return "black timeout";
    default:
      return "unknown";
    }
  }

  std::optional<Chess::Move> parse_move_input(const std::string&                 user_input,
                                              const Chess::ChessGameHandler&      handler,
                                              const Chess::ChessTranslationUnit&   translation_unit)
  {
    const std::string trimmed = trim_copy(user_input);
    if (trimmed.empty())
    {
      return std::nullopt;
    }

    const std::string lowered = to_lower_copy(trimmed);
    const std::string normalized_input_san = strip_san_suffixes(trimmed);
    const std::string lowered_normalized_san = to_lower_copy(normalized_input_san);

    const Chess::Grid& grid = handler.get_current_grid();
    const auto legal_moves = handler.get_legal_moves();

    for (const auto& legal_move : legal_moves)
    {
      const std::string long_alg = to_lower_copy(translation_unit.move_to_algebraic(legal_move, grid));
      if (!long_alg.empty() && long_alg == lowered)
      {
        return legal_move;
      }

      const std::string san = translation_unit.move_to_san(legal_move, grid);
      if (san == trimmed || strip_san_suffixes(san) == normalized_input_san || to_lower_copy(san) == lowered
          || to_lower_copy(strip_san_suffixes(san)) == lowered_normalized_san)
      {
        return legal_move;
      }
    }

    return std::nullopt;
  }

  void print_help()
  {
    std::cout << "Commands:\n";
    std::cout << "  help                 Show this help\n";
    std::cout << "  board                Print board\n";
    std::cout << "  state                Print game state\n";
    std::cout << "  fen                  Print current FEN\n";
    std::cout << "  loadfen <fen>        Load position from FEN\n";
    std::cout << "  moves                List legal moves (SAN and long algebraic)\n";
    std::cout << "  ai [depth]           Ask engine for a move and play it\n";
    std::cout << "  new                  Start a new game\n";
    std::cout << "  quit                 Exit interactive mode\n";
    std::cout << "  <move>               Play move in SAN or long algebraic (e.g. e2e4, Nf3)\n";
  }
} // namespace

int main()
{
  Chess::ChessGameHandler game_handler;
  Chess::ChessEngine chess_engine;
  Chess::ChessTranslationUnit translation_unit;

  game_handler.start_new_game();

  std::cout << "=====================================\n";
  std::cout << " Inheritance Chess Interactive Mode\n";
  std::cout << "=====================================\n";
  std::cout << "Type 'help' to view commands.\n\n";
  game_handler.display_board();

  bool running = true;
  while (running)
  {
    const Chess::Color side_to_move = game_handler.get_current_grid().current_turn;
    std::cout << "\n" << color_to_string(side_to_move) << " to move > ";

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

    std::istringstream line_stream(line);
    std::string command;
    line_stream >> command;
    const std::string lowered_command = to_lower_copy(command);

    if (lowered_command == "help")
    {
      print_help();
      continue;
    }

    if (lowered_command == "quit" || lowered_command == "exit")
    {
      running = false;
      continue;
    }

    if (lowered_command == "board")
    {
      game_handler.display_board();
      continue;
    }

    if (lowered_command == "state")
    {
      std::cout << "Game state: " << game_state_to_string(game_handler.get_game_state()) << "\n";
      continue;
    }

    if (lowered_command == "fen")
    {
      std::cout << game_handler.get_current_fen() << "\n";
      continue;
    }

    if (lowered_command == "new")
    {
      game_handler.start_new_game();
      std::cout << "Started a new game.\n";
      game_handler.display_board();
      continue;
    }

    if (lowered_command == "loadfen")
    {
      std::string fen;
      std::getline(line_stream >> std::ws, fen);
      fen = trim_copy(fen);
      if (fen.empty())
      {
        std::cout << "Usage: loadfen <fen-string>\n";
        continue;
      }

      try
      {
        game_handler.load_game_from_fen(fen);
        std::cout << "FEN loaded successfully.\n";
        game_handler.display_board();
      }
      catch (const std::exception& e)
      {
        std::cout << "Invalid FEN: " << e.what() << "\n";
      }
      continue;
    }

    if (lowered_command == "moves")
    {
      const Chess::Grid& grid = game_handler.get_current_grid();
      const auto legal_moves = game_handler.get_legal_moves();
      std::cout << "Legal moves: " << legal_moves.size() << "\n";

      for (const auto& move : legal_moves)
      {
        const std::string san = translation_unit.move_to_san(move, grid);
        const std::string long_alg = translation_unit.move_to_algebraic(move, grid);
        std::cout << "  " << san << " (" << long_alg << ")\n";
      }
      continue;
    }

    if (lowered_command == "ai")
    {
      int depth = 3;
      if (!(line_stream >> depth))
      {
        depth = 3;
      }
      if (depth < 1)
      {
        depth = 1;
      }

      Chess::SearchLimits limits;
      limits.max_depth = depth;
      limits.max_time = std::chrono::milliseconds(1000);

      const auto result = chess_engine.find_best_move(game_handler.get_current_grid(), limits);
      const std::string played_san = translation_unit.move_to_san(result.best_move, game_handler.get_current_grid());
      const std::string played_long_alg = translation_unit.move_to_algebraic(result.best_move, game_handler.get_current_grid());

      if (!game_handler.make_move(result.best_move))
      {
        std::cout << "Engine suggested an invalid move for current state.\n";
        continue;
      }

      std::cout << "Engine played: " << played_san << " (" << played_long_alg << ")\n";
      std::cout << "Score: " << result.score << " cp, depth: " << result.depth
                << ", nodes: " << result.nodes_searched << ", time: " << result.time_used.count() << " ms\n";
      game_handler.display_board();
    }
    else
    {
      const auto parsed = parse_move_input(line, game_handler, translation_unit);
      if (!parsed.has_value())
      {
        std::cout << "Unknown command or illegal move: '" << line << "'. Type 'help'.\n";
        continue;
      }

      const std::string played_san = translation_unit.move_to_san(*parsed, game_handler.get_current_grid());
      if (!game_handler.make_move(*parsed))
      {
        std::cout << "Illegal move: " << line << "\n";
        continue;
      }

      std::cout << "Played: " << played_san << "\n";
      game_handler.display_board();
    }

    const Chess::GameState state = game_handler.get_game_state();
    if (state != Chess::GameState::ONGOING)
    {
      std::cout << "Game finished: " << game_state_to_string(state) << "\n";
      std::cout << "Use 'new' to start another game or 'quit' to exit.\n";
    }
  }

  std::cout << "Thanks for playing Inheritance Chess.\n";

  return 0;
}
