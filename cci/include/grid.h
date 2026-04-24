/// @file      grid.h
/// @namespace Chess
/// @brief     Chess Common Interface (CCI) - Grid Structure.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   Defines the Grid struct which represents the complete chess board
///            state with 8x8 piece positions and game metadata.

#ifndef ICHESS_CCI_GRID
#define ICHESS_CCI_GRID

#include "chess_types.h"
#include "position.h"
#include <array>
#include <optional>
#include <vector>
#include <memory>

namespace Chess
{

  // Forward declarations for polymorphic piece interface
  class ChessPiece;

  /// @brief Forward declaration of Grid class for use in ChessPiece interface.
  class Grid;

  /// @class ChessPiece::List
  /// @brief A vector of raw pointers to chess pieces.
  /// @note  This container holds references to pieces. Memory is managed
  ///        by the pieces_ member vector. (Adapted from existing Piece::List)
  using PieceList = std::vector<ChessPiece*>;

  /// @class ChessPiece::PositionList
  /// @brief A vector of chess pieces positions. (Adapted from existing Piece::PositionList)
  using PositionList = std::vector<Position>;

  /// @class ChessPiece::ColorList
  /// @brief A vector of chess pieces colors. (Adapted from existing Piece::ColorList)
  using ColorList = std::vector<Color>;

  /// @class ChessPiece
  /// @brief Abstract base class for all chess pieces using polymorphism.
  /// @details This class serves as the polymorphic base for all chess pieces,
  ///          adapted from the existing Piece class to work with CCI namespace.
  class ChessPiece
  {
    public:
      /// @brief Virtual destructor for proper cleanup in derived classes.
      virtual ~ChessPiece() = default;

      /// @brief Get the type of the piece.
      virtual PieceType get_type() const = 0;

      /// @brief Get the color of the piece.
      virtual Color get_color() const = 0;

      /// @brief Get the current position of the piece.
      virtual Position get_position() const = 0;

      /// @brief Set the position of the piece.
      virtual void set_position(const Position& pos) = 0;

      /// @brief Get the character representation of the piece.
      virtual char get_representation() const = 0;

      /// @brief Calculate valid moves for this piece using PieceList parameter.
      /// @param[out] moves Vector to be filled with valid move positions.
      /// @param[in] other Vector of unique pointers to all other pieces on the board.
      /// @param[in] grid Current grid state for move validation.
      /// @details This method extracts positions and colors from the PieceList and calls
      ///          the pure virtual overload. Adapted from existing Piece::available_moves.
      void available_moves(PositionList& moves, const PieceList& other, const Grid& grid) const;

      /// @brief Pure virtual method to calculate valid moves for the piece.
      /// @param[out] moves Vector to be filled with valid move positions.
      /// @param[in] other_positions Vector of positions of all other pieces on the board.
      /// @param[in] other_colors Vector of colors corresponding to each piece in other_positions.
      /// @param[in] grid Current grid state for move validation.
      /// @note Must be implemented by derived classes for their specific movement rules.
      /// @details This is the primary method that derived classes must implement.
      virtual void available_moves(PositionList&       moves,
                                   const PositionList& other_positions,
                                   const ColorList&    other_colors,
                                   const Grid&         grid) const = 0;
  };

  /// @brief Factory function for creating chess pieces.
  /// @param type The type of piece to create.
  /// @param color The color of the piece.
  /// @param pos The initial position of the piece.
  /// @return Unique pointer to the created piece.
  /// @details Factory pattern adapted from existing piece creation patterns.
  std::unique_ptr<ChessPiece> create_piece(PieceType type, Color color, const Position& pos);

  /// @struct PieceProperties
  /// @brief  Represents a piece on the board with its metadata.
  struct PieceProperties
  {
      PieceType type  = PieceType::PAWN;
      Color     color = Color::WHITE;
      Position  position;
      bool      has_moved             = false; ///< Used for castling and pawn double-move rules
      bool      en_passant_vulnerable = false; ///< Set when pawn can be captured en passant

      /// @brief Check if square is empty (default state).
      bool is_empty() const { return type == PieceType::PAWN && color == Color::WHITE && !has_moved; }
  };

  /// @struct Grid
  /// @brief  Represents the complete state of a chess board.
  /// @details The grid is an 8x8 array where board[file][rank] represents a square.
  ///          File: 0-7 (a-h, columns), Rank: 0-7 (1-8, rows).
  struct Grid
  {
      std::array<std::array<std::optional<PieceProperties>, 8>, 8> board;

      Color     current_turn = Color::WHITE; ///< Whose turn it is
      GameFlags flags;                       ///< Castling rights, en passant, halfmove/fullmove counters

    public:
      // Delete copy constructor and assignment operator to prevent copying of unique_ptr vectors
      Grid() = default;
      Grid(const Grid&) = delete;
      Grid& operator=(const Grid&) = delete;
      Grid(Grid&&) = default;
      Grid& operator=(Grid&&) = default;
      /// @brief Initialize grid to standard starting position.
      void initialize_standard_position();

      /// @brief Get piece at a specific position (if any).
      const std::optional<PieceProperties>& get_piece(const Position& pos) const
      {
        if (!pos.is_valid())
        {
          static const std::optional<PieceProperties> empty;
          return empty;
        }
        return board[pos.file][pos.rank];
      }

      /// @brief Check if a square is occupied.
      bool is_occupied(const Position& pos) const
      {
        if (!pos.is_valid())
          return false;
        return board[pos.file][pos.rank].has_value();
      }

      /**
       * @brief Switch the current turn
       */
      void switch_turn() { current_turn = (current_turn == Color::WHITE) ? Color::BLACK : Color::WHITE; }

      /// @brief Clear a square (remove the piece).
      void clear_square(const Position& pos)
      {
        if (pos.is_valid())
        {
          board[pos.file][pos.rank] = std::nullopt;
        }
      }

      /// @brief Get all pieces on the board as a PieceList.
      /// @return Vector of references to all pieces on the board.
      /// @details Adapted from existing Board::pieces pattern.
      std::vector<ChessPiece*> get_all_pieces() const;

      /// @brief Add a piece to the board.
      /// @param piece Unique pointer to the piece to add.
      /// @details Takes ownership of the piece. Adapted from existing Board patterns.
      void add_piece(std::unique_ptr<ChessPiece> piece);

      /// @brief Remove a piece from the board.
      /// @param pos Position of the piece to remove.
      /// @return True if piece was removed, false if no piece at position.
      bool remove_piece(const Position& pos);

      /// @brief Get piece at a specific position (if any).
      /// @param pos Position to check.
      /// @return Raw pointer to piece at position, or nullptr if empty.
      /// @details Adapted from existing Board piece access patterns.
      ChessPiece* get_piece_at(const Position& pos);

      /// @brief Get piece at a specific position (const version).
      /// @param pos Position to check.
      /// @return Const raw pointer to piece at position, or nullptr if empty.
      const ChessPiece* get_piece_at(const Position& pos) const;

      /// @brief Convert grid state to PieceProperties array for compatibility.
      /// @details Bridges the gap between PieceList and grid representation.
      void update_piece_properties();

    private:
      std::vector<std::unique_ptr<ChessPiece>> pieces_; ///< List of all pieces on the board (adapted from existing Board::pieces)

      /// @brief Set piece at a specific position.
      void set_piece(const Position& pos, const std::optional<PieceProperties>& piece)
      {
        if (pos.is_valid())
        {
          board[pos.file][pos.rank] = piece;
        }
      }

      /// @brief Check if a square is attacked by a color (to be implemented by CBM).
      bool is_square_attacked_by(const Position& pos, Color attacking_color) const;
  };

} // namespace Chess

#endif // ICHESS_CCI_GRID
