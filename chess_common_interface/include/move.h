/// @file      move.h
/// @namespace Chess
/// @brief     Chess Common Interface (CCI) - Move Structure.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines the Move struct to represent a chess move with all
///            associated metadata (source, destination, flags, promotion).

#ifndef ICHESS_CCI_MOVE
#define ICHESS_CCI_MOVE

#include "position.h"
#include "chess_types.h"

namespace Chess
{

  /// @struct Move
  /// @brief  Represents a single chess move.
  /// @details A move consists of a source position, destination position, optional
  ///          special flags (castling, en passant, promotion), and promotion piece type.
  struct Move
  {
      Position     start_pos;                            ///< Source square
      Position     end_pos;                              ///< Destination square
      SpecialFlags flags           = SpecialFlags::NONE; ///< Special move flags
      PieceType    promotion_piece = PieceType::QUEEN;   ///< Promotion piece (if applicable)

      /// @brief Default constructor.
      Move() = default;

      /// @brief Constructor with start and end positions.
      Move(Position start, Position end) : start_pos(start), end_pos(end) {}

      /// @brief Constructor with start, end, and special flags.
      Move(Position start, Position end, SpecialFlags f) : start_pos(start), end_pos(end), flags(f) {}

      /// @brief Constructor with all fields.
      Move(Position start, Position end, SpecialFlags f, PieceType promotion)
          : start_pos(start), end_pos(end), flags(f), promotion_piece(promotion)
      {
      }

      /// @brief Check if move is a promotion.
      bool is_promotion() const { return flags == SpecialFlags::PROMOTION; }

      /// @brief Check if move is castling.
      bool is_castling() const
      {
        return flags == SpecialFlags::CASTLE_KINGSIDE || flags == SpecialFlags::CASTLE_QUEENSIDE;
      }

      /// @brief Check if move is en passant.
      bool is_en_passant() const { return flags == SpecialFlags::EN_PASSANT; }

      /// @brief Equality operator.
      bool operator==(const Move& other) const
      {
        return start_pos == other.start_pos && end_pos == other.end_pos && flags == other.flags
               && promotion_piece == other.promotion_piece;
      }

      /// @brief Inequality operator.
      bool operator!=(const Move& other) const { return !(*this == other); }
  };

} // namespace Chess

#endif // ICHESS_CCI_MOVE
