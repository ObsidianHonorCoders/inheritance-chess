/// @file      chess_types.h
/// @namespace Chess
/// @brief     Chess Common Interface (CCI) - Type Definitions and Enums.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines common data types, enumerations, and structures used across
///            all chess engine modules. Centralizes type definitions to ensure
///            consistency and enable global modifications without affecting
///            individual modules.

#ifndef ICHESS_CCI_CHESS_TYPES
#define ICHESS_CCI_CHESS_TYPES

namespace Chess
{

  /// @enum  Color
  /// @brief Represents the color/side of a chess piece.
  enum class Color
  {
    WHITE = 0,
    BLACK = 1
  };

  /// @enum  PieceType
  /// @brief Represents the type of chess piece.
  enum class PieceType
  {
    PAWN   = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK   = 3,
    QUEEN  = 4,
    KING   = 5
  };

  /// @enum  SpecialFlags
  /// @brief Represents special move flags (castling, en passant, promotion).
  enum class SpecialFlags
  {
    NONE             = 0,
    CASTLE_KINGSIDE  = 1,
    CASTLE_QUEENSIDE = 2,
    EN_PASSANT       = 3,
    PROMOTION        = 4
  };

  /// @enum  GameState
  /// @brief Represents the current state of a chess game.
  enum class GameState
  {
    ONGOING                    = 0,
    CHECKMATE                  = 1,
    STALEMATE                  = 2,
    DRAW_INSUFFICIENT_MATERIAL = 3,
    DRAW_THREEFOLD_REPETITION  = 4,
    DRAW_FIFTY_MOVE_RULE       = 5,
    WHITE_RESIGNED             = 6,
    BLACK_RESIGNED             = 7,
    DRAW_BY_AGREEMENT          = 8,
    WHITE_TIMEOUT              = 9,
    BLACK_TIMEOUT              = 10
  };

  /// @struct GameFlags
  /// @brief  Tracks flags for castling rights and other game state.
  struct GameFlags
  {
      bool white_can_castle_kingside  = true;
      bool white_can_castle_queenside = true;
      bool black_can_castle_kingside  = true;
      bool black_can_castle_queenside = true;
      int  halfmove_clock             = 0; ///< For fifty-move rule
      int  fullmove_number            = 1; ///< Full move count
  };

} // namespace Chess

#endif // ICHESS_CCI_CHESS_TYPES
