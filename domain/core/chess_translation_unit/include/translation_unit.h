/// @file      translation_unit.h
/// @namespace Chess
/// @brief     Chess Translation Unit (CTU) - The Adapter.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Translates between external chess notations (FEN, algebraic) and
///            the engine's internal representations. Allows interfacing with
///            different UIs and external chess engines.

#ifndef ICHESS_CTU_TRANSLATION_UNIT
#define ICHESS_CTU_TRANSLATION_UNIT

#include "grid.h"
#include "move.h"
#include <string>
#include <utility>
#include <vector>

namespace Chess
{

  class ChessGameHandler;
  class ChessEngine;

  struct PGNVariation;

  struct PGNMoveNode
  {
    Move                     move;
    std::string              san;
    std::vector<std::string> comments;
    std::vector<std::string> annotations;
    std::vector<PGNVariation> variations;
  };

  struct PGNVariation
  {
    std::vector<PGNMoveNode> moves;
  };

  struct PGNGame
  {
    std::vector<std::pair<std::string, std::string>> tags;
    std::vector<std::string>                          malformed_tag_lines;
    std::vector<PGNMoveNode>                          moves;
    std::string                                       result = "*";
  };

  struct PGNParseOptions
  {
    bool recover_malformed_tags = true;
    bool strict_tag_parsing = false;
  };

  struct PGNSerializeOptions
  {
    bool strict_normalization = false;
    bool sort_tags = false;
    bool normalize_annotations = false;
  };

  struct ProtocolAdapterState
  {
    bool force_mode = false;
    int  max_depth = 20;
    int  move_time_ms = 5000;
    bool infinite = false;
  };

  struct ProtocolCommandResult
  {
    std::vector<std::string> responses;
    bool                     quit = false;
  };

  /// @class   ChessTranslationUnit
  /// @brief   Converts between external chess notations and internal formats.
  /// @details The ChessTranslationUnit translates between FEN notation, algebraic
  ///          notation, and internal Grid/Move representations, enabling
  ///          interface with various chess UI systems and external engines.
  class ChessTranslationUnit
  {
    public:
      /// @brief Convert FEN string to internal Grid representation.
      Grid fen_to_internal(const std::string& fen_string) const;

      /// @brief Convert internal Grid to FEN string.
      std::string internal_to_fen(const Grid& grid) const;

      /// @brief Convert algebraic notation to Move (requires board context).
      Move algebraic_to_move(const std::string& algebraic, const Grid& grid) const;

      /// @brief Convert Move to algebraic notation.
      std::string move_to_algebraic(const Move& move, const Grid& grid) const;

      /// @brief Convert SAN notation to Move (requires board context).
      Move san_to_move(const std::string& san, const Grid& grid) const;

      /// @brief Convert Move to SAN notation.
      std::string move_to_san(const Move& move, const Grid& grid) const;

      /// @brief Render board to ASCII for CLI presentation.
      std::string render_board_ascii(const Grid& grid) const;

      /// @brief Convert move list to PGN move text.
      std::string moves_to_pgn(const std::vector<Move>& moves, const Grid& initial_grid, const std::string& result = "*") const;

      /// @brief Parse PGN move text into move list.
      std::vector<Move> pgn_to_moves(const std::string& pgn, const Grid& initial_grid) const;

      /// @brief Parse rich PGN text including tags, comments, and recursive variations.
      PGNGame parse_pgn_game(const std::string& pgn,
                 const Grid& initial_grid,
                 const PGNParseOptions& options = PGNParseOptions{}) const;

      /// @brief Serialize rich PGN game including tags, comments, and variations.
      std::string pgn_game_to_string(const PGNGame& game,
                 const Grid& initial_grid,
                 const PGNSerializeOptions& options = PGNSerializeOptions{}) const;

    private:
      /// @brief Convert character to PieceType.
      PieceType char_to_piece_type(char c) const;
      /// @brief Convert character to Color.
      Color char_to_color(char c) const;
      /// @brief Convert algebraic position string to Position.
      Position algebraic_to_position(const std::string& pos) const;
      /// @brief Convert PieceType and Color to character.
      char piece_type_to_char(PieceType type, Color color) const;
  };

  class ChessProtocolAdapter
  {
    public:
      ProtocolCommandResult handle_uci_command(const std::string& command,
                                               ChessGameHandler&  handler,
                                               ChessEngine&       engine) const;

      ProtocolCommandResult handle_xboard_command(const std::string& command,
                                                  ChessGameHandler&  handler,
                                                  ChessEngine&       engine,
                                                  ProtocolAdapterState& state) const;
  };

} // namespace Chess

#endif // ICHESS_CTU_TRANSLATION_UNIT
