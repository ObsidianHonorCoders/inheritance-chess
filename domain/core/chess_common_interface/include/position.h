/// @file      position.h
/// @namespace Chess
/// @brief     Chess Common Interface (CCI) - Position Structure.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines the Position struct for representing a single square
///            on the 8x8 chess board.

#ifndef ICHESS_CCI_POSITION
#define ICHESS_CCI_POSITION

#include <cstdint>
#include <functional>
#include <string>

namespace Chess
{

  /**
   * @struct Position
   * @brief Represents a single square on an 8x8 chess board
   *
   * Positions are indexed from 0-7:
   * - file: 0-7 (a-h, left to right)
   * - rank: 0-7 (1-8, bottom to top)
   */
  struct Position
  {
      int file = 0; ///< File index (0-7, where 0=a, 7=h)
      int rank = 0; ///< Rank index (0-7, where 0=1, 7=8)

      /// @brief Default constructor - initializes to a1 (0,0).
      Position() = default;

      /// @brief Constructor with file and rank parameters.
      Position(int f, int r) : file(f), rank(r) {}

      /// @brief Check if position is valid (0-7 range).
      bool is_valid() const { return file >= 0 && file <= 7 && rank >= 0 && rank <= 7; }

      /// @brief Equality operator.
      bool operator==(const Position& other) const { return file == other.file && rank == other.rank; }

      /// @brief Inequality operator.
      bool operator!=(const Position& other) const { return !(*this == other); }

      /// @brief Less-than operator for sorting/containers.
      bool operator<(const Position& other) const
      {
        if (file != other.file)
          return file < other.file;
        return rank < other.rank;
      }
  };

  /// @brief Convert Position to algebraic notation (e.g., "e2", "a8").
  /// @param pos Position to convert.
  /// @return String representation in algebraic notation.
  std::string position_to_algebraic(const Position& pos);

  /// @brief Convert algebraic notation to Position.
  /// @param algebraic Algebraic notation (e.g., "e2", "a8").
  /// @return Position object.
  /// @throws std::invalid_argument if algebraic notation is invalid.
  Position algebraic_to_position(const std::string& algebraic);

} // namespace Chess

/// @brief Hash function for Position to use in std::unordered_set/map.
template <>
struct std::hash<Chess::Position>
{
    std::size_t operator()(const Chess::Position& pos) const { return std::hash<int>()(pos.file * 8 + pos.rank); }
};

#endif // ICHESS_CCI_POSITION
