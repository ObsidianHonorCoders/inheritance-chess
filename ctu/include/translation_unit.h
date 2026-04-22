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

namespace Chess
{

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

} // namespace Chess

#endif // ICHESS_CTU_TRANSLATION_UNIT
