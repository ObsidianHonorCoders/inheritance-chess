/// @file      translation_unit.cpp
/// @namespace Chess
/// @brief     Implementation for Chess Translation Unit (CTU).
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Implements notation parsing and conversion utilities.

#include "translation_unit.h"
#include "board_manager.h"
#include "chess_engine.h"
#include "game_handler.h"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <functional>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace Chess
{

  namespace
  {
    std::string trim_copy(const std::string& input)
    {
      const auto start = input.find_first_not_of(" \t\n\r");
      if (start == std::string::npos)
      {
        return "";
      }

      const auto end = input.find_last_not_of(" \t\n\r");
      return input.substr(start, end - start + 1);
    }

    std::string strip_san_suffixes(std::string san)
    {
      while (!san.empty())
      {
        const char c = san.back();
        if (c == '+' || c == '#' || c == '!' || c == '?')
        {
          san.pop_back();
          continue;
        }
        break;
      }
      return san;
    }

    std::string position_to_string(const Position& pos)
    {
      std::string out;
      out += static_cast<char>('a' + pos.file);
      out += static_cast<char>('1' + pos.rank);
      return out;
    }

    bool is_capture_move(const Grid& grid, const Move& move)
    {
      if (move.is_en_passant())
      {
        return true;
      }

      const auto destination_piece = grid.get_piece(move.end_pos);
      if (!destination_piece.has_value())
      {
        return false;
      }

      const auto source_piece = grid.get_piece(move.start_pos);
      if (!source_piece.has_value())
      {
        return false;
      }

      return destination_piece->color != source_piece->color;
    }

    char piece_letter(PieceType type)
    {
      switch (type)
      {
      case PieceType::KNIGHT:
        return 'N';
      case PieceType::BISHOP:
        return 'B';
      case PieceType::ROOK:
        return 'R';
      case PieceType::QUEEN:
        return 'Q';
      case PieceType::KING:
        return 'K';
      case PieceType::PAWN:
      default:
        return '\0';
      }
    }

    char promotion_letter(PieceType type)
    {
      switch (type)
      {
      case PieceType::KNIGHT:
        return 'N';
      case PieceType::BISHOP:
        return 'B';
      case PieceType::ROOK:
        return 'R';
      case PieceType::QUEEN:
        return 'Q';
      default:
        return '\0';
      }
    }

    bool is_result_token(const std::string& token)
    {
      return token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*";
    }

    bool is_move_number_token(const std::string& token)
    {
      static const std::regex move_number_regex("^[0-9]+\\.(\\.\\.)?$");
      return std::regex_match(token, move_number_regex);
    }

    bool is_nag_token(const std::string& token)
    {
      static const std::regex nag_regex("^\\$[0-9]+$");
      return std::regex_match(token, nag_regex);
    }

    bool is_symbolic_annotation_token(const std::string& token)
    {
      return token == "!" || token == "?" || token == "!!" || token == "??" || token == "!?" || token == "?!";
    }

    std::pair<std::string, std::string> split_inline_numeric_nag(const std::string& token)
    {
      const auto pos = token.find('$');
      if (pos == std::string::npos || pos == 0 || pos + 1 >= token.size())
      {
        return {token, ""};
      }

      for (std::size_t i = pos + 1; i < token.size(); ++i)
      {
        if (!std::isdigit(static_cast<unsigned char>(token[i])))
        {
          return {token, ""};
        }
      }

      return {token.substr(0, pos), token.substr(pos)};
    }

    std::pair<std::string, std::string> split_trailing_symbolic_annotation(const std::string& token)
    {
      static const std::vector<std::string> annotations = {"!!", "??", "!?", "?!", "!", "?"};
      for (const auto& ann : annotations)
      {
        if (token.size() > ann.size() && token.compare(token.size() - ann.size(), ann.size(), ann) == 0)
        {
          return {token.substr(0, token.size() - ann.size()), ann};
        }
      }
      return {token, ""};
    }

    std::string normalize_annotation_token(const std::string& token)
    {
      if (token == "!")
        return "$1";
      if (token == "?")
        return "$2";
      if (token == "!!")
        return "$3";
      if (token == "??")
        return "$4";
      if (token == "!?")
        return "$5";
      if (token == "?!")
        return "$6";
      return token;
    }

    std::string normalize_space_runs(const std::string& input)
    {
      std::string out;
      bool last_space = false;
      for (char c : input)
      {
        if (std::isspace(static_cast<unsigned char>(c)))
        {
          if (!last_space)
          {
            out += ' ';
            last_space = true;
          }
          continue;
        }

        out += c;
        last_space = false;
      }
      return trim_copy(out);
    }

    bool parse_tag_line_relaxed(const std::string& line, std::pair<std::string, std::string>& out_tag)
    {
      const std::string trimmed = trim_copy(line);
      if (trimmed.size() < 2 || trimmed.front() != '[')
      {
        return false;
      }

      std::size_t i = 1;
      while (i < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[i])))
      {
        ++i;
      }

      const std::size_t key_start = i;
      while (i < trimmed.size() && !std::isspace(static_cast<unsigned char>(trimmed[i])) && trimmed[i] != ']')
      {
        ++i;
      }

      if (i == key_start)
      {
        return false;
      }

      const std::string key = trimmed.substr(key_start, i - key_start);
      while (i < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[i])))
      {
        ++i;
      }

      std::string value;
      if (i < trimmed.size() && trimmed[i] == '"')
      {
        ++i;
        bool escaping = false;
        while (i < trimmed.size())
        {
          const char c = trimmed[i++];
          if (escaping)
          {
            value += c;
            escaping = false;
            continue;
          }
          if (c == '\\')
          {
            escaping = true;
            continue;
          }
          if (c == '"')
          {
            break;
          }
          value += c;
        }
      }
      else
      {
        const std::size_t close = trimmed.find(']', i);
        value = (close == std::string::npos) ? trimmed.substr(i) : trimmed.substr(i, close - i);
        value = trim_copy(value);
      }

      out_tag = {key, value};
      return true;
    }

    std::string strip_inline_move_number(const std::string& token)
    {
      const auto dot_pos = token.find_last_of('.');
      if (dot_pos == std::string::npos || dot_pos + 1 >= token.size())
      {
        return token;
      }

      bool numeric_prefix = true;
      for (std::size_t i = 0; i <= dot_pos; ++i)
      {
        const char c = token[i];
        if (!(std::isdigit(static_cast<unsigned char>(c)) || c == '.'))
        {
          numeric_prefix = false;
          break;
        }
      }

      return numeric_prefix ? token.substr(dot_pos + 1) : token;
    }

    std::string escape_pgn_value(const std::string& value)
    {
      std::string escaped;
      for (char c : value)
      {
        if (c == '\\' || c == '"')
        {
          escaped += '\\';
        }
        escaped += c;
      }
      return escaped;
    }

    std::string unescape_pgn_value(const std::string& value)
    {
      std::string unescaped;
      bool escaping = false;
      for (char c : value)
      {
        if (escaping)
        {
          unescaped += c;
          escaping = false;
          continue;
        }

        if (c == '\\')
        {
          escaping = true;
          continue;
        }

        unescaped += c;
      }
      return unescaped;
    }

    struct PGNToken
    {
        enum class Type
        {
            Symbol,
            Comment,
            VariationStart,
            VariationEnd
        } type = Type::Symbol;
        std::string text;
    };

    std::vector<PGNToken> tokenize_pgn_movetext(const std::string& movetext)
    {
      std::vector<PGNToken> tokens;
      std::size_t i = 0;
      while (i < movetext.size())
      {
        const char c = movetext[i];
        if (std::isspace(static_cast<unsigned char>(c)))
        {
          ++i;
          continue;
        }

        if (c == '{')
        {
          ++i;
          std::string comment;
          while (i < movetext.size() && movetext[i] != '}')
          {
            comment += movetext[i++];
          }
          if (i < movetext.size() && movetext[i] == '}')
          {
            ++i;
          }
          tokens.push_back({PGNToken::Type::Comment, trim_copy(comment)});
          continue;
        }

        if (c == ';')
        {
          ++i;
          while (i < movetext.size() && movetext[i] != '\n')
          {
            ++i;
          }
          continue;
        }

        if (c == '(')
        {
          tokens.push_back({PGNToken::Type::VariationStart, "("});
          ++i;
          continue;
        }

        if (c == ')')
        {
          tokens.push_back({PGNToken::Type::VariationEnd, ")"});
          ++i;
          continue;
        }

        std::string symbol;
        while (i < movetext.size())
        {
          const char inner = movetext[i];
          if (std::isspace(static_cast<unsigned char>(inner)) || inner == '{' || inner == '}' || inner == '(' || inner == ')' || inner == ';')
          {
            break;
          }
          symbol += inner;
          ++i;
        }

        if (!symbol.empty())
        {
          tokens.push_back({PGNToken::Type::Symbol, symbol});
        }
      }

      return tokens;
    }

    std::vector<std::string> split_command_tokens(const std::string& command)
    {
      std::istringstream iss(command);
      std::vector<std::string> tokens;
      std::string token;
      while (iss >> token)
      {
        tokens.push_back(token);
      }
      return tokens;
    }
  } // namespace

  /// @brief Parse a six-field FEN string into a grid.
  Grid ChessTranslationUnit::fen_to_internal(const std::string& fen_string) const
  {
    Grid grid;

    // Split FEN string into 6 fields
    std::vector<std::string> fields;
    std::string current_field;
    for (char c : fen_string)
    {
      if (c == ' ')
      {
        if (!current_field.empty())
        {
          fields.push_back(current_field);
          current_field.clear();
        }
      }
      else
      {
        current_field += c;
      }
    }
    if (!current_field.empty())
    {
      fields.push_back(current_field);
    }

    if (fields.size() < 6)
    {
      throw std::invalid_argument("Invalid FEN: must have 6 fields");
    }

    // Field 1: Piece placement (from rank 8 down to rank 1)
    const std::string& placement = fields[0];
    int current_rank = 7; // Start from rank 8 (index 7)
    int current_file = 0; // Start from file a (index 0)

    for (char c : placement)
    {
      if (c == '/')
      {
        // Move to next rank
        if (current_file != 8)
        {
          throw std::invalid_argument("Invalid FEN: incomplete rank");
        }
        current_rank--;
        current_file = 0;
      }
      else if (std::isdigit(c))
      {
        // Empty squares
        int empty_count = c - '0';
        current_file += empty_count;
      }
      else if (std::isalpha(c))
      {
        // Piece
        if (current_file >= 8 || current_rank < 0)
        {
          throw std::invalid_argument("Invalid FEN: piece placement overflow");
        }

        PieceType type = char_to_piece_type(c);
        Color color = char_to_color(c);
        Position pos(current_file, current_rank);
        
        // Create piece with has_moved flag set appropriately
        // Pieces in starting position have not moved (white pawns on rank 1, black on rank 6, etc.)
        bool has_moved = false;
        if (type == PieceType::PAWN && ((color == Color::WHITE && current_rank == 1) || (color == Color::BLACK && current_rank == 6)))
        {
          has_moved = false;
        }
        else if (type == PieceType::KING && ((color == Color::WHITE && current_rank == 0 && current_file == 4) || 
                                              (color == Color::BLACK && current_rank == 7 && current_file == 4)))
        {
          has_moved = false;
        }
        else if (type == PieceType::ROOK && ((color == Color::WHITE && current_rank == 0 && (current_file == 0 || current_file == 7)) ||
                                              (color == Color::BLACK && current_rank == 7 && (current_file == 0 || current_file == 7))))
        {
          has_moved = false;
        }
        else
        {
          has_moved = true; // Any piece not in starting position has moved
        }

        auto piece = create_piece(type, color, pos);
        grid.add_piece(std::move(piece));

        // Mark piece as having moved if not in starting position
        if (has_moved)
        {
          grid.set_has_moved(pos);
        }

        current_file++;
      }
      else
      {
        throw std::invalid_argument("Invalid FEN: invalid character in placement");
      }
    }

    // After parsing full placement, we should end on rank index 0 with file advanced to 8.
    if (current_rank != 0 || current_file != 8)
    {
      throw std::invalid_argument("Invalid FEN: incomplete piece placement");
    }

    // Field 2: Active color
    const std::string& active_color = fields[1];
    if (active_color == "w")
    {
      grid.current_turn = Color::WHITE;
    }
    else if (active_color == "b")
    {
      grid.current_turn = Color::BLACK;
    }
    else
    {
      throw std::invalid_argument("Invalid FEN: active color must be 'w' or 'b'");
    }

    // Field 3: Castling rights
    const std::string& castling = fields[2];
    grid.flags.white_can_castle_kingside = false;
    grid.flags.white_can_castle_queenside = false;
    grid.flags.black_can_castle_kingside = false;
    grid.flags.black_can_castle_queenside = false;

    if (castling != "-")
    {
      for (char c : castling)
      {
        switch (c)
        {
        case 'K':
          grid.flags.white_can_castle_kingside = true;
          break;
        case 'Q':
          grid.flags.white_can_castle_queenside = true;
          break;
        case 'k':
          grid.flags.black_can_castle_kingside = true;
          break;
        case 'q':
          grid.flags.black_can_castle_queenside = true;
          break;
        default:
          throw std::invalid_argument("Invalid FEN: invalid castling rights character");
        }
      }
    }

    auto sync_has_moved = [&grid](const Position& pos, PieceType expected_type, Color expected_color, bool has_moved)
    {
      const auto piece = grid.get_piece(pos);
      if (!piece.has_value() || piece->type != expected_type || piece->color != expected_color)
      {
        return;
      }

      PieceProperties updated = piece.value();
      updated.has_moved = has_moved;
      grid.set_piece(pos, updated);
    };

    // Reconstruct king/rook move-state from castling rights for round-trip consistency.
    sync_has_moved(Position(4, 0), PieceType::KING, Color::WHITE,
                   !(grid.flags.white_can_castle_kingside || grid.flags.white_can_castle_queenside));
    sync_has_moved(Position(4, 7), PieceType::KING, Color::BLACK,
                   !(grid.flags.black_can_castle_kingside || grid.flags.black_can_castle_queenside));
    sync_has_moved(Position(0, 0), PieceType::ROOK, Color::WHITE, !grid.flags.white_can_castle_queenside);
    sync_has_moved(Position(7, 0), PieceType::ROOK, Color::WHITE, !grid.flags.white_can_castle_kingside);
    sync_has_moved(Position(0, 7), PieceType::ROOK, Color::BLACK, !grid.flags.black_can_castle_queenside);
    sync_has_moved(Position(7, 7), PieceType::ROOK, Color::BLACK, !grid.flags.black_can_castle_kingside);

    // Field 4: En passant target square
    const std::string& en_passant = fields[3];
    if (en_passant != "-")
    {
      // Parse en passant position (e.g., "e3")
      Position ep_pos = algebraic_to_position(en_passant);
      if (ep_pos.is_valid())
      {
        // Mark the pawn that can be captured en passant as vulnerable
        // The en passant square is where the capturing pawn would land, so the vulnerable pawn is:
        // - For white capturing: one rank below the en passant square
        // - For black capturing: one rank above the en passant square
        int vulnerable_rank = (grid.current_turn == Color::WHITE) ? ep_pos.rank - 1 : ep_pos.rank + 1;
        if (vulnerable_rank >= 0 && vulnerable_rank <= 7)
        {
          Position vulnerable_pos(ep_pos.file, vulnerable_rank);
          auto vulnerable_piece_opt = grid.get_piece(vulnerable_pos);
          if (vulnerable_piece_opt.has_value() && vulnerable_piece_opt->type == PieceType::PAWN)
          {
            // Use new public method to mark piece as en passant vulnerable
            grid.set_en_passant_vulnerable(vulnerable_pos);
          }
        }
      }
    }

    // Field 5: Halfmove clock
    const std::string& halfmove = fields[4];
    try
    {
      grid.flags.halfmove_clock = std::stoi(halfmove);
    }
    catch (const std::exception&)
    {
      throw std::invalid_argument("Invalid FEN: halfmove clock must be a number");
    }

    // Field 6: Fullmove number
    const std::string& fullmove = fields[5];
    try
    {
      grid.flags.fullmove_number = std::stoi(fullmove);
      if (grid.flags.fullmove_number < 1)
      {
        throw std::invalid_argument("Invalid FEN: fullmove number must be >= 1");
      }
    }
    catch (const std::exception&)
    {
      throw std::invalid_argument("Invalid FEN: fullmove number must be a number");
    }

    return grid;
  }

  /// @brief Serialize a grid into a six-field FEN string.
  std::string ChessTranslationUnit::internal_to_fen(const Grid& grid) const
  {
    std::string fen;

    // Build piece placement (from rank 7 down to 0, representing rows 8-1)
    for (int rank = 7; rank >= 0; --rank)
    {
      int empty_count = 0;

      for (int file = 0; file < 8; ++file)
      {
        const auto& piece_opt = grid.get_piece(Position(file, rank));

        if (piece_opt.has_value())
        {
          // Add empty squares count if any
          if (empty_count > 0)
          {
            fen += std::to_string(empty_count);
            empty_count = 0;
          }

          // Add piece character
          char piece_char;
          switch (piece_opt->type)
          {
          case PieceType::PAWN:
            piece_char = 'p';
            break;
          case PieceType::KNIGHT:
            piece_char = 'n';
            break;
          case PieceType::BISHOP:
            piece_char = 'b';
            break;
          case PieceType::ROOK:
            piece_char = 'r';
            break;
          case PieceType::QUEEN:
            piece_char = 'q';
            break;
          case PieceType::KING:
            piece_char = 'k';
            break;
          default:
            piece_char = '?';
          }

          // Uppercase for white pieces
          if (piece_opt->color == Color::WHITE)
          {
            piece_char = static_cast<char>(std::toupper(static_cast<unsigned char>(piece_char)));
          }

          fen += piece_char;
        }
        else
        {
          empty_count++;
        }
      }

      // Add remaining empty squares count
      if (empty_count > 0)
      {
        fen += std::to_string(empty_count);
      }

      // Add rank separator
      if (rank > 0)
      {
        fen += '/';
      }
    }

    // Add active color
    fen += (grid.current_turn == Color::WHITE) ? " w " : " b ";

    // Add castling rights
    std::string castling;
    if (grid.flags.white_can_castle_kingside)
      castling += 'K';
    if (grid.flags.white_can_castle_queenside)
      castling += 'Q';
    if (grid.flags.black_can_castle_kingside)
      castling += 'k';
    if (grid.flags.black_can_castle_queenside)
      castling += 'q';
    if (castling.empty())
      castling = '-';

    fen += castling;

    // Add en passant target square
    Position ep_target = grid.get_en_passant_target();
    if (ep_target.is_valid())
    {
      fen += " ";
      fen += static_cast<char>('a' + ep_target.file);
      fen += static_cast<char>('1' + ep_target.rank);
    }
    else
    {
      fen += " -";
    }

    // Add halfmove and fullmove counters
    fen += " ";
    fen += std::to_string(grid.flags.halfmove_clock) + " ";
    fen += std::to_string(grid.flags.fullmove_number);

    return fen;
  }

  /// @brief Implement algebraic to move.
  Move ChessTranslationUnit::algebraic_to_move(const std::string& algebraic, const Grid& grid) const
  {
    (void)grid;
    // Handle long algebraic notation: e2e4, a7a5, etc.
    // Format: source_file (a-h) + source_rank (1-8) + dest_file (a-h) + dest_rank (1-8)

    if (algebraic.length() == 4 || algebraic.length() == 5)
    {
      // Parse source square
      char src_file_char = static_cast<char>(std::tolower(static_cast<unsigned char>(algebraic[0])));
      char src_rank_char = algebraic[1];

      // Parse destination square
      char dst_file_char = static_cast<char>(std::tolower(static_cast<unsigned char>(algebraic[2])));
      char dst_rank_char = algebraic[3];

      // Validate characters
      if (src_file_char >= 'a' && src_file_char <= 'h' && src_rank_char >= '1' && src_rank_char <= '8'
          && dst_file_char >= 'a' && dst_file_char <= 'h' && dst_rank_char >= '1' && dst_rank_char <= '8')
      {
        // Convert to 0-based indices
        // Files: a-h -> 0-7
        // Ranks: 1-8 -> 0-7 (rank 1 is index 0, rank 8 is index 7)
        int src_file = src_file_char - 'a';
        int src_rank = src_rank_char - '1';
        int dst_file = dst_file_char - 'a';
        int dst_rank = dst_rank_char - '1';

        if (algebraic.length() == 5)
        {
          const char promo_char = static_cast<char>(std::tolower(static_cast<unsigned char>(algebraic[4])));
          PieceType promotion_piece;
          switch (promo_char)
          {
          case 'q':
            promotion_piece = PieceType::QUEEN;
            break;
          case 'r':
            promotion_piece = PieceType::ROOK;
            break;
          case 'b':
            promotion_piece = PieceType::BISHOP;
            break;
          case 'n':
            promotion_piece = PieceType::KNIGHT;
            break;
          default:
            return Move();
          }

          return Move(Position(src_file, src_rank), Position(dst_file, dst_rank), SpecialFlags::PROMOTION, promotion_piece);
        }

        return Move(Position(src_file, src_rank), Position(dst_file, dst_rank));
      }
    }

    // Fallback: accept SAN-like input when long algebraic is not provided.
    return san_to_move(algebraic, grid);
  }

  /// @brief Implement move to algebraic.
  std::string ChessTranslationUnit::move_to_algebraic(const Move& move, const Grid& grid) const
  {
    (void)grid;
    // Convert move to long algebraic notation: e2e4, a7a5, etc.
    // Format: source_file (a-h) + source_rank (1-8) + dest_file (a-h) + dest_rank (1-8)

    std::string result;

    // Validate move positions
    if (!move.start_pos.is_valid() || !move.end_pos.is_valid())
    {
      return ""; // Invalid move
    }

    // Convert source position to algebraic notation
    result += static_cast<char>('a' + move.start_pos.file); // file (a-h)
    result += static_cast<char>('1' + move.start_pos.rank); // rank (1-8)

    // Convert destination position to algebraic notation
    result += static_cast<char>('a' + move.end_pos.file); // file (a-h)
    result += static_cast<char>('1' + move.end_pos.rank); // rank (1-8)

    if (move.is_promotion())
    {
      switch (move.promotion_piece)
      {
      case PieceType::QUEEN:
        result += 'q';
        break;
      case PieceType::ROOK:
        result += 'r';
        break;
      case PieceType::BISHOP:
        result += 'b';
        break;
      case PieceType::KNIGHT:
        result += 'n';
        break;
      default:
        return "";
      }
    }

    return result;
  }

  /// @brief Implement san to move.
  Move ChessTranslationUnit::san_to_move(const std::string& san, const Grid& grid) const
  {
    const std::string trimmed = trim_copy(san);
    if (trimmed.empty())
    {
      return Move();
    }

    std::string token = strip_san_suffixes(trimmed);

    ChessBoardManager manager;
    const auto legal_moves = manager.get_legal_moves(grid);

    if (token == "O-O" || token == "0-0")
    {
      for (const auto& move : legal_moves)
      {
        if (move.flags == SpecialFlags::CASTLE_KINGSIDE)
        {
          return move;
        }
      }
      return Move();
    }

    if (token == "O-O-O" || token == "0-0-0")
    {
      for (const auto& move : legal_moves)
      {
        if (move.flags == SpecialFlags::CASTLE_QUEENSIDE)
        {
          return move;
        }
      }
      return Move();
    }

    PieceType piece_type = PieceType::PAWN;
    std::size_t cursor = 0;
    if (!token.empty())
    {
      switch (token[0])
      {
      case 'K':
        piece_type = PieceType::KING;
        cursor = 1;
        break;
      case 'Q':
        piece_type = PieceType::QUEEN;
        cursor = 1;
        break;
      case 'R':
        piece_type = PieceType::ROOK;
        cursor = 1;
        break;
      case 'B':
        piece_type = PieceType::BISHOP;
        cursor = 1;
        break;
      case 'N':
        piece_type = PieceType::KNIGHT;
        cursor = 1;
        break;
      default:
        break;
      }
    }

    std::size_t promotion_pos = token.find('=');
    PieceType promotion_piece = PieceType::QUEEN;
    bool requires_promotion = false;
    if (promotion_pos != std::string::npos)
    {
      if (promotion_pos + 1 >= token.size())
      {
        return Move();
      }

      const char p = token[promotion_pos + 1];
      switch (p)
      {
      case 'Q':
        promotion_piece = PieceType::QUEEN;
        break;
      case 'R':
        promotion_piece = PieceType::ROOK;
        break;
      case 'B':
        promotion_piece = PieceType::BISHOP;
        break;
      case 'N':
        promotion_piece = PieceType::KNIGHT;
        break;
      default:
        return Move();
      }
      requires_promotion = true;
      token = token.substr(0, promotion_pos);
    }

    if (token.size() < 2)
    {
      return Move();
    }

    const std::string destination = token.substr(token.size() - 2, 2);
    const Position destination_pos = algebraic_to_position(destination);
    if (!destination_pos.is_valid())
    {
      return Move();
    }

    const std::string core = token.substr(cursor, token.size() - cursor - 2);
    const bool requires_capture = core.find('x') != std::string::npos;

    char disambiguation_file = '\0';
    char disambiguation_rank = '\0';
    for (char c : core)
    {
      if (c == 'x')
      {
        continue;
      }
      if (c >= 'a' && c <= 'h')
      {
        disambiguation_file = c;
      }
      else if (c >= '1' && c <= '8')
      {
        disambiguation_rank = c;
      }
    }

    std::vector<Move> candidates;
    for (const auto& move : legal_moves)
    {
      if (move.end_pos != destination_pos)
      {
        continue;
      }

      const auto source_piece = grid.get_piece(move.start_pos);
      if (!source_piece.has_value() || source_piece->type != piece_type)
      {
        continue;
      }

      if (requires_capture && !is_capture_move(grid, move))
      {
        continue;
      }

      if (!requires_capture && is_capture_move(grid, move))
      {
        continue;
      }

      if (requires_promotion)
      {
        if (!move.is_promotion() || move.promotion_piece != promotion_piece)
        {
          continue;
        }
      }

      if (piece_type == PieceType::PAWN && requires_capture && disambiguation_file != '\0')
      {
        if (move.start_pos.file != disambiguation_file - 'a')
        {
          continue;
        }
      }

      if (disambiguation_file != '\0' && piece_type != PieceType::PAWN)
      {
        if (move.start_pos.file != disambiguation_file - 'a')
        {
          continue;
        }
      }

      if (disambiguation_rank != '\0')
      {
        if (move.start_pos.rank != disambiguation_rank - '1')
        {
          continue;
        }
      }

      candidates.push_back(move);
    }

    if (candidates.size() == 1)
    {
      return candidates.front();
    }

    return Move();
  }

  /// @brief Implement move to san.
  std::string ChessTranslationUnit::move_to_san(const Move& move, const Grid& grid) const
  {
    if (!move.start_pos.is_valid() || !move.end_pos.is_valid())
    {
      return "";
    }

    const auto source_piece = grid.get_piece(move.start_pos);
    if (!source_piece.has_value())
    {
      return "";
    }

    ChessBoardManager manager;
    const auto legal_moves = manager.get_legal_moves(grid);

    bool is_legal = false;
    for (const auto& legal_move : legal_moves)
    {
      if (legal_move == move)
      {
        is_legal = true;
        break;
      }
    }
    if (!is_legal)
    {
      return "";
    }

    std::string san;

    if (move.flags == SpecialFlags::CASTLE_KINGSIDE)
    {
      san = "O-O";
    }
    else if (move.flags == SpecialFlags::CASTLE_QUEENSIDE)
    {
      san = "O-O-O";
    }
    else
    {
      const PieceType type = source_piece->type;
      const bool capture = is_capture_move(grid, move);

      if (type != PieceType::PAWN)
      {
        san += piece_letter(type);
      }

      if (type == PieceType::PAWN)
      {
        if (capture)
        {
          san += static_cast<char>('a' + move.start_pos.file);
        }
      }
      else
      {
        std::vector<Move> ambiguous;
        for (const auto& candidate : legal_moves)
        {
          if (candidate.end_pos != move.end_pos || candidate.start_pos == move.start_pos)
          {
            continue;
          }

          const auto candidate_piece = grid.get_piece(candidate.start_pos);
          if (!candidate_piece.has_value() || candidate_piece->type != type)
          {
            continue;
          }

          ambiguous.push_back(candidate);
        }

        if (!ambiguous.empty())
        {
          bool same_file_exists = false;
          bool same_rank_exists = false;
          for (const auto& candidate : ambiguous)
          {
            if (candidate.start_pos.file == move.start_pos.file)
            {
              same_file_exists = true;
            }
            if (candidate.start_pos.rank == move.start_pos.rank)
            {
              same_rank_exists = true;
            }
          }

          if (!same_file_exists)
          {
            san += static_cast<char>('a' + move.start_pos.file);
          }
          else if (!same_rank_exists)
          {
            san += static_cast<char>('1' + move.start_pos.rank);
          }
          else
          {
            san += static_cast<char>('a' + move.start_pos.file);
            san += static_cast<char>('1' + move.start_pos.rank);
          }
        }
      }

      if (capture)
      {
        san += 'x';
      }

      san += position_to_string(move.end_pos);

      if (move.is_promotion())
      {
        const char promo = promotion_letter(move.promotion_piece);
        if (promo == '\0')
        {
          return "";
        }
        san += '=';
        san += promo;
      }
    }

    Grid next_grid = grid.clone();
    next_grid.apply_move_inplace(move);

    const Color side_to_move = next_grid.current_turn;
    if (manager.is_checkmate(next_grid, side_to_move))
    {
      san += '#';
    }
    else if (manager.is_king_in_check(next_grid, side_to_move))
    {
      san += '+';
    }

    return san;
  }

  /// @brief Render board ascii.
  std::string ChessTranslationUnit::render_board_ascii(const Grid& grid) const
  {
    std::ostringstream oss;
    oss << "\n  a b c d e f g h\n";
    oss << " +-----------------+\n";

    for (int rank = 7; rank >= 0; --rank)
    {
      oss << rank + 1 << "|";
      for (int file = 0; file < 8; ++file)
      {
        const auto piece = grid.get_piece(Position(file, rank));
        if (piece.has_value())
        {
          char piece_char = piece_type_to_char(piece->type, piece->color);
          if (piece->color == Color::WHITE)
          {
            piece_char = static_cast<char>(std::toupper(static_cast<unsigned char>(piece_char)));
          }
          else
          {
            piece_char = static_cast<char>(std::tolower(static_cast<unsigned char>(piece_char)));
          }
          oss << ' ' << piece_char;
        }
        else
        {
          oss << " .";
        }
      }
      oss << " |\n";
    }

    oss << " +-----------------+\n";
    oss << "Turn: " << (grid.current_turn == Color::WHITE ? "White" : "Black") << "\n";

    return oss.str();
  }

  /// @brief Implement moves to pgn.
  std::string ChessTranslationUnit::moves_to_pgn(const std::vector<Move>& moves, const Grid& initial_grid, const std::string& result) const
  {
    PGNGame game;
    game.result = result;
    Grid working_grid = initial_grid.clone();

    for (const auto& move : moves)
    {
      PGNMoveNode node;
      node.move = move;
      node.san = move_to_san(move, working_grid);
      if (node.san.empty())
      {
        return "";
      }
      game.moves.push_back(node);
      working_grid.apply_move_inplace(move);
    }

    return pgn_game_to_string(game, initial_grid);
  }

  /// @brief Implement pgn to moves.
  std::vector<Move> ChessTranslationUnit::pgn_to_moves(const std::string& pgn, const Grid& initial_grid) const
  {
    PGNGame game = parse_pgn_game(pgn, initial_grid);
    std::vector<Move> moves;
    moves.reserve(game.moves.size());
    for (const auto& node : game.moves)
    {
      moves.push_back(node.move);
    }
    return moves;
  }

  /// @brief Parse pgn game.
  PGNGame ChessTranslationUnit::parse_pgn_game(const std::string& pgn,
                                               const Grid& initial_grid,
                                               const PGNParseOptions& options) const
  {
    PGNGame game;

    std::ostringstream movetext_stream;
    std::istringstream pgn_stream(pgn);
    std::string line;
    const std::regex tag_regex("^\\s*\\[([^\\s]+)\\s+\"((?:[^\"\\\\]|\\\\.)*)\"\\]\\s*$");
    while (std::getline(pgn_stream, line))
    {
      std::smatch match;
      if (std::regex_match(line, match, tag_regex))
      {
        game.tags.emplace_back(match[1].str(), unescape_pgn_value(match[2].str()));
      }
      else if (options.recover_malformed_tags)
      {
        std::pair<std::string, std::string> recovered_tag;
        if (parse_tag_line_relaxed(line, recovered_tag))
        {
          game.tags.push_back(recovered_tag);
          game.malformed_tag_lines.push_back(line);
        }
        else
        {
          const std::string trimmed = trim_copy(line);
          if (options.strict_tag_parsing && !trimmed.empty() && trimmed.front() == '[')
          {
            return PGNGame{};
          }
          movetext_stream << line << '\n';
        }
      }
      else
      {
        const std::string trimmed = trim_copy(line);
        if (options.strict_tag_parsing && !trimmed.empty() && trimmed.front() == '[')
        {
          return PGNGame{};
        }
        movetext_stream << line << '\n';
      }
    }

    const auto tokens = tokenize_pgn_movetext(movetext_stream.str());
    std::size_t index = 0;

    std::function<bool(std::size_t&, Grid&, std::vector<PGNMoveNode>&, bool)> parse_sequence;
    parse_sequence = [&](std::size_t& cursor, Grid& working_grid, std::vector<PGNMoveNode>& out_moves, bool top_level) -> bool
    {
      Grid last_move_base_grid;
      bool have_last_move_base = false;
      PGNMoveNode* last_move = nullptr;

      while (cursor < tokens.size())
      {
        const auto& token = tokens[cursor];
        if (token.type == PGNToken::Type::VariationEnd)
        {
          ++cursor;
          return true;
        }

        if (token.type == PGNToken::Type::VariationStart)
        {
          ++cursor;
          PGNVariation variation;
          Grid branch_grid = have_last_move_base ? last_move_base_grid.clone() : working_grid.clone();
          if (!parse_sequence(cursor, branch_grid, variation.moves, false))
          {
            return false;
          }
          if (last_move != nullptr)
          {
            last_move->variations.push_back(variation);
          }
          continue;
        }

        if (token.type == PGNToken::Type::Comment)
        {
          if (last_move != nullptr && !token.text.empty())
          {
            last_move->comments.push_back(token.text);
          }
          ++cursor;
          continue;
        }

        std::string symbol = strip_inline_move_number(token.text);
        if (symbol.empty() || is_move_number_token(symbol))
        {
          ++cursor;
          continue;
        }

        if (is_nag_token(symbol) || is_symbolic_annotation_token(symbol))
        {
          if (last_move != nullptr)
          {
            last_move->annotations.push_back(symbol);
          }
          ++cursor;
          continue;
        }

        if (is_result_token(symbol))
        {
          if (top_level)
          {
            game.result = symbol;
          }
          ++cursor;
          return true;
        }

        auto split_nag = split_inline_numeric_nag(symbol);
        std::string move_symbol = split_nag.first;
        const std::string inline_nag = split_nag.second;

        auto split_symbolic = split_trailing_symbolic_annotation(move_symbol);
        move_symbol = split_symbolic.first;
        const std::string inline_symbolic_annotation = split_symbolic.second;

        if (move_symbol.empty())
        {
          return false;
        }

        Grid base_grid = working_grid.clone();
        Move move = san_to_move(move_symbol, working_grid);
        if (!move.start_pos.is_valid() || !move.end_pos.is_valid())
        {
          return false;
        }

        PGNMoveNode node;
        node.move = move;
        node.san = move_to_san(move, working_grid);
        if (node.san.empty())
        {
          node.san = move_symbol;
        }
        if (!inline_nag.empty())
        {
          node.annotations.push_back(inline_nag);
        }
        if (!inline_symbolic_annotation.empty())
        {
          node.annotations.push_back(inline_symbolic_annotation);
        }
        out_moves.push_back(node);
        last_move = &out_moves.back();
        last_move_base_grid = base_grid.clone();
        have_last_move_base = true;

        working_grid.apply_move_inplace(move);
        ++cursor;
      }

      return true;
    };

    Grid root_grid = initial_grid.clone();
    if (!parse_sequence(index, root_grid, game.moves, true))
    {
      return PGNGame{};
    }

    return game;
  }

  /// @brief Implement pgn game to string.
  std::string ChessTranslationUnit::pgn_game_to_string(const PGNGame& game,
                                                       const Grid& initial_grid,
                                                       const PGNSerializeOptions& options) const
  {
    PGNSerializeOptions normalized_options = options;
    if (normalized_options.strict_normalization)
    {
      normalized_options.sort_tags = true;
      normalized_options.normalize_annotations = true;
    }

    std::vector<std::pair<std::string, std::string>> tags = game.tags;
    const auto normalized_result = is_result_token(game.result) ? game.result : "*";

    if (normalized_options.strict_normalization)
    {
      bool has_result_tag = false;
      for (auto& tag : tags)
      {
        if (tag.first == "Result")
        {
          tag.second = normalized_result;
          has_result_tag = true;
        }
      }
      if (!has_result_tag)
      {
        tags.emplace_back("Result", normalized_result);
      }
    }

    if (normalized_options.sort_tags)
    {
      const auto tag_rank = [](const std::string& key) {
        if (key == "Event")
          return 0;
        if (key == "Site")
          return 1;
        if (key == "Date")
          return 2;
        if (key == "Round")
          return 3;
        if (key == "White")
          return 4;
        if (key == "Black")
          return 5;
        if (key == "Result")
          return 6;
        return 100;
      };

      std::stable_sort(tags.begin(), tags.end(), [&](const auto& a, const auto& b) {
        const int ra = tag_rank(a.first);
        const int rb = tag_rank(b.first);
        if (ra != rb)
        {
          return ra < rb;
        }
        return a.first < b.first;
      });
    }

    std::ostringstream oss;
    for (const auto& tag : tags)
    {
      oss << '[' << tag.first << " \"" << escape_pgn_value(tag.second) << "\"]\n";
    }
    if (!tags.empty())
    {
      oss << '\n';
    }

    std::function<void(const std::vector<PGNMoveNode>&, Grid&, std::ostringstream&)> serialize_sequence;
    serialize_sequence = [&](const std::vector<PGNMoveNode>& moves, Grid& working_grid, std::ostringstream& out)
    {
      int move_number = working_grid.flags.fullmove_number;
      if (move_number < 1)
      {
        move_number = 1;
      }

      for (std::size_t i = 0; i < moves.size(); ++i)
      {
        if (i > 0)
        {
          out << ' ';
        }

        Grid pre_move_grid = working_grid.clone();
        if (working_grid.current_turn == Color::WHITE)
        {
          out << move_number << ". ";
        }
        else if (i == 0)
        {
          out << move_number << "... ";
        }

        const std::string san = !moves[i].san.empty() ? moves[i].san : move_to_san(moves[i].move, working_grid);
        out << san;

        for (const auto& annotation : moves[i].annotations)
        {
          const std::string token = normalized_options.normalize_annotations
                                        ? normalize_annotation_token(annotation)
                                        : annotation;
          if (!token.empty())
          {
            out << ' ' << token;
          }
        }

        for (const auto& comment : moves[i].comments)
        {
          const std::string normalized_comment = normalized_options.strict_normalization
                                                     ? normalize_space_runs(comment)
                                                     : comment;
          out << " {" << normalized_comment << '}';
        }

        working_grid.apply_move_inplace(moves[i].move);

        for (const auto& variation : moves[i].variations)
        {
          out << " (";
          std::ostringstream variation_stream;
          Grid variation_grid = pre_move_grid.clone();
          serialize_sequence(variation.moves, variation_grid, variation_stream);
          out << trim_copy(variation_stream.str()) << ')';
        }

        if (pre_move_grid.current_turn == Color::BLACK)
        {
          ++move_number;
        }
      }
    };

    Grid root_grid = initial_grid.clone();
    serialize_sequence(game.moves, root_grid, oss);
    if (!normalized_result.empty())
    {
      const std::string movetext = trim_copy(oss.str());
      oss.str("");
      oss.clear();
      if (!movetext.empty())
      {
        oss << movetext << ' ';
      }
      oss << normalized_result;
    }

    return trim_copy(oss.str());
  }

  /// @brief Implement handle uci command.
  ProtocolCommandResult ChessProtocolAdapter::handle_uci_command(const std::string& command,
                                                                 ChessGameHandler&  handler,
                                                                 ChessEngine&       engine) const
  {
    ProtocolCommandResult result;
    ChessTranslationUnit translator;
    const auto tokens = split_command_tokens(command);
    if (tokens.empty())
    {
      return result;
    }

    if (tokens[0] == "uci")
    {
      result.responses.push_back("id name Inheritance Chess Engine");
      result.responses.push_back("id author Obsidian Honor Coders");
      result.responses.push_back("uciok");
      return result;
    }

    if (tokens[0] == "isready")
    {
      result.responses.push_back("readyok");
      return result;
    }

    if (tokens[0] == "ucinewgame")
    {
      handler.start_new_game();
      return result;
    }

    if (tokens[0] == "position")
    {
      std::size_t i = 1;
      if (i < tokens.size() && tokens[i] == "startpos")
      {
        handler.start_new_game();
        ++i;
      }
      else if (i < tokens.size() && tokens[i] == "fen")
      {
        ++i;
        if (i + 5 >= tokens.size())
        {
          return result;
        }
        std::ostringstream fen;
        for (int field = 0; field < 6; ++field, ++i)
        {
          if (field > 0)
          {
            fen << ' ';
          }
          fen << tokens[i];
        }
        handler.load_game_from_fen(fen.str());
      }

      if (i < tokens.size() && tokens[i] == "moves")
      {
        ++i;
        for (; i < tokens.size(); ++i)
        {
          handler.make_move(tokens[i]);
        }
      }
      return result;
    }

    if (tokens[0] == "go")
    {
      SearchLimits limits;
      for (std::size_t i = 1; i < tokens.size(); ++i)
      {
        if (tokens[i] == "depth" && i + 1 < tokens.size())
        {
          limits.max_depth = std::stoi(tokens[++i]);
        }
        else if (tokens[i] == "movetime" && i + 1 < tokens.size())
        {
          limits.max_time = std::chrono::milliseconds(std::stoi(tokens[++i]));
        }
        else if (tokens[i] == "infinite")
        {
          limits.infinite = true;
        }
      }

      const auto evaluation = engine.find_best_move(handler.get_current_grid(), limits);
      result.responses.push_back("bestmove " + translator.move_to_algebraic(evaluation.best_move, handler.get_current_grid()));
      return result;
    }

    if (tokens[0] == "quit")
    {
      result.quit = true;
    }

    return result;
  }

  /// @brief Implement handle xboard command.
  ProtocolCommandResult ChessProtocolAdapter::handle_xboard_command(const std::string& command,
                                                                    ChessGameHandler&  handler,
                                                                    ChessEngine&       engine,
                                                                    ProtocolAdapterState& state) const
  {
    ProtocolCommandResult result;
    ChessTranslationUnit translator;
    const auto tokens = split_command_tokens(command);
    if (tokens.empty())
    {
      return result;
    }

    if (tokens[0] == "xboard")
    {
      return result;
    }

    if (tokens[0] == "protover")
    {
      result.responses.push_back("feature ping=1 setboard=1 usermove=1 done=1 san=1");
      return result;
    }

    if (tokens[0] == "new")
    {
      handler.start_new_game();
      state.force_mode = false;
      return result;
    }

    if (tokens[0] == "force")
    {
      state.force_mode = true;
      return result;
    }

    if (tokens[0] == "setboard")
    {
      const auto fen_pos = command.find(' ');
      if (fen_pos != std::string::npos)
      {
        handler.load_game_from_fen(trim_copy(command.substr(fen_pos + 1)));
      }
      return result;
    }

    if (tokens[0] == "sd" && tokens.size() > 1)
    {
      state.max_depth = std::stoi(tokens[1]);
      return result;
    }

    if (tokens[0] == "st" && tokens.size() > 1)
    {
      state.move_time_ms = static_cast<int>(std::stod(tokens[1]) * 1000.0);
      return result;
    }

    if (tokens[0] == "ping" && tokens.size() > 1)
    {
      result.responses.push_back("pong " + tokens[1]);
      return result;
    }

    const auto produce_engine_move = [&]() {
      SearchLimits limits;
      limits.max_depth = state.max_depth;
      limits.max_time = std::chrono::milliseconds(state.move_time_ms);
      limits.infinite = state.infinite;
      const auto evaluation = engine.find_best_move(handler.get_current_grid(), limits);
      const std::string move_text = translator.move_to_algebraic(evaluation.best_move, handler.get_current_grid());
      if (!move_text.empty())
      {
        handler.make_move(move_text);
        result.responses.push_back("move " + move_text);
      }
    };

    if (tokens[0] == "usermove" && tokens.size() > 1)
    {
      handler.make_move(tokens[1]);
      if (!state.force_mode)
      {
        produce_engine_move();
      }
      return result;
    }

    if (tokens[0] == "go")
    {
      state.force_mode = false;
      produce_engine_move();
      return result;
    }

    if (tokens[0] == "quit")
    {
      result.quit = true;
    }

    return result;
  }

  /// @brief Implement char to piece type.
  PieceType ChessTranslationUnit::char_to_piece_type(char c) const
  {
    switch (std::tolower(c))
    {
    case 'p':
      return PieceType::PAWN;
    case 'n':
      return PieceType::KNIGHT;
    case 'b':
      return PieceType::BISHOP;
    case 'r':
      return PieceType::ROOK;
    case 'q':
      return PieceType::QUEEN;
    case 'k':
      return PieceType::KING;
    default:
      throw std::invalid_argument("Invalid piece character");
    }
  }

  /// @brief Implement char to color.
  Color ChessTranslationUnit::char_to_color(char c) const { return std::isupper(c) ? Color::WHITE : Color::BLACK; }

  Position ChessTranslationUnit::algebraic_to_position(const std::string& pos) const
  {
    if (pos.length() != 2)
    {
      return Position(-1, -1); // Invalid position
    }

    char file_char = static_cast<char>(std::tolower(static_cast<unsigned char>(pos[0])));
    char rank_char = pos[1];

    // Validate file (a-h) and rank (1-8)
    if (file_char < 'a' || file_char > 'h' || rank_char < '1' || rank_char > '8')
    {
      return Position(-1, -1); // Invalid position
    }

    // Convert to 0-based indices
    int file = file_char - 'a';    // a-h -> 0-7
    int rank = rank_char - '1';    // 1-8 -> 0-7

    return Position(file, rank);
  }

  /// @brief Implement piece type to char.
  char ChessTranslationUnit::piece_type_to_char(PieceType type, Color color) const
  {
    char base;
    switch (type)
    {
    case PieceType::PAWN:
      base = 'P';
      break;
    case PieceType::KNIGHT:
      base = 'N';
      break;
    case PieceType::BISHOP:
      base = 'B';
      break;
    case PieceType::ROOK:
      base = 'R';
      break;
    case PieceType::QUEEN:
      base = 'Q';
      break;
    case PieceType::KING:
      base = 'K';
      break;
    default:
      base = '?';
      break;
    }
    return (color == Color::WHITE) ? base : (char)std::tolower(base);
  }

} // namespace Chess


