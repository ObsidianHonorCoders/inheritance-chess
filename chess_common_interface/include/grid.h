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
#include "move.h"
#include "position.h"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include <memory>

namespace Chess
{

  // Forward declarations for polymorphic piece interface
  class ChessPiece;

  /// @brief Forward declaration of Grid type for use in ChessPiece interface.
  struct Grid;

  /// @brief A vector of raw pointers to chess pieces.
  /// @note  Holds non-owning references; heap ownership lives in the pieces_
  ///        unique_ptr store inside Grid.
  using PieceList = std::vector<ChessPiece*>;

  /// @brief A vector of board positions, used by available_moves() overloads.
  using PositionList = std::vector<Position>;

  /// @brief A vector of piece colors, parallel to PositionList in move-generation calls.
  using ColorList = std::vector<Color>;

  /// @class ChessPiece
  /// @brief Abstract base class for all chess pieces using polymorphism.
  /// @details Polymorphic base for all CPL piece types. Derived classes implement
  ///          available_moves() for their piece-specific movement rules.
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
      /// @details Extracts positions and colors from the PieceList and calls
      ///          the pure virtual overload with those parallel vectors.
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
  /// @details Dispatches on PieceType to construct the correct concrete subclass
  ///          and returns it as a heap-allocated unique_ptr<ChessPiece>.
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

  /// @struct UndoRecord
  /// @brief  Captures minimal state for undoing a move in-place.
  struct UndoRecord
  {
      std::optional<PieceProperties> captured_piece;  ///< Piece at destination before move
      std::optional<PieceProperties> en_passant_pawn; ///< Pawn captured via en passant (if applicable)
      Position rook_original_pos;                     ///< Original rook position (if castling)
      std::array<bool, 64> en_passant_vulnerable_before{}; ///< En passant vulnerability map before move
      GameFlags flags_before;                         ///< Game flags before move
      Color turn_before;                              ///< Whose turn it was before move
  };

  /// @struct Grid
  /// @brief  Represents the complete state of a chess board.
  /// @details The grid is an 8x8 array where board[file][rank] represents a square.
  ///          File: 0-7 (a-h, columns), Rank: 0-7 (1-8, rows).
  struct Grid
  {
      Color     current_turn = Color::WHITE; ///< Whose turn it is
      GameFlags flags;                       ///< Castling rights, en passant, halfmove/fullmove counters

    public:
      // Delete copy constructor and assignment operator to prevent copying of unique_ptr vectors
      Grid();
      Grid(const Grid&) = delete;
      Grid& operator=(const Grid&) = delete;
      Grid(Grid&&) = default;
      Grid& operator=(Grid&&) = default;
      /// @brief Initialize grid to standard starting position.
      void initialize_standard_position();

      /// @brief Get piece at a specific position (if any).
      std::optional<PieceProperties> get_piece(const Position& pos) const;

      /// @brief Get all piece properties on the board (authoritative value-type API).
      /// @return Vector of all PieceProperties on the board.
      /// @details Returns the authoritative value-type representation of all pieces.
      ///          This is the preferred API for internal operations. OO adapters via
      ///          get_all_pieces() are provided for external consumers only.
      std::vector<PieceProperties> get_all_piece_properties() const;

      /// @brief Check if a square is occupied.
      bool is_occupied(const Position& pos) const;

      /**
       * @brief Switch the current turn
       */
      void switch_turn() { current_turn = (current_turn == Color::WHITE) ? Color::BLACK : Color::WHITE; }

      /// @brief Clear a square (remove the piece).
      void clear_square(const Position& pos);

      /// @brief Get all pieces on the board as a PieceList.
      /// @return Vector of references to all pieces on the board.
      /// @details Returns raw pointers into the internally owned pieces; caller
      ///          must not delete them.
      std::vector<ChessPiece*> get_all_pieces() const;

      /// @brief Add a piece to the board.
      /// @param piece Unique pointer to the piece to add.
      /// @details Transfers ownership into the internal unique_ptr store and
      ///          registers the piece in the positional index.
      void add_piece(std::unique_ptr<ChessPiece> piece);

      /// @brief Remove a piece from the board.
      /// @param pos Position of the piece to remove.
      /// @return True if piece was removed, false if no piece at position.
      bool remove_piece(const Position& pos);

      /// @brief Get piece at a specific position (if any).
      /// @param pos Position to check.
      /// @return Raw pointer to piece at position, or nullptr if empty.
      /// @details Looks up the positional index and returns a non-owning raw pointer,
      ///          or nullptr when the square is empty.
      ChessPiece* get_piece_at(const Position& pos);

      /// @brief Get piece at a specific position (const version).
      /// @param pos Position to check.
      /// @return Const raw pointer to piece at position, or nullptr if empty.
      const ChessPiece* get_piece_at(const Position& pos) const;

      /// @brief Convert grid state to PieceProperties array for compatibility.
      /// @details Bridges the gap between PieceList and grid representation.
      void update_piece_properties();

      /// @brief Set or clear piece at a specific position (production mutation operation).
      /// @param pos Position to place or clear piece.
      /// @param piece Piece properties to place, or std::nullopt to clear the square.
      /// @details This is the core mutation operation for the Grid API. It is used internally
      ///          by apply_move and undo_move, and by orchestration layers for special moves
      ///          (castling, en passant, promotion) and position setup.
      void set_piece(const Position& pos, const std::optional<PieceProperties>& piece);

      /// @brief Deep clone this grid state for copy-apply search.
      Grid clone() const;

      /// @brief Apply a move in-place and return undo record (fast path for search).
      /// @param move Move to apply.
      /// @return UndoRecord that can be used to restore state.
      UndoRecord apply_move_inplace(const Move& move);

      /// @brief Undo a move using an UndoRecord (restores grid state).
      /// @param move Move that was applied (needed to identify pieces).
      /// @param undo_record Record from apply_move_inplace.
      void undo_move(const Move& move, const UndoRecord& undo_record);

      /// @brief Return occupancy bitboard for all pieces.
      uint64_t occupancy_mask() const { return occupied_mask_; }

      /// @brief Return occupancy bitboard for one side.
      uint64_t occupancy_mask(Color color) const
      {
        return (color == Color::WHITE) ? white_occupied_mask_ : black_occupied_mask_;
      }

      /// @brief Mark a piece as having moved (for FEN round-trip semantics).
      /// @param pos Position of the piece to mark.
      void set_has_moved(const Position& pos);

      /// @brief Mark a piece as vulnerable to en passant capture (for FEN round-trip semantics).
      /// @param pos Position of the piece to mark.
      void set_en_passant_vulnerable(const Position& pos);

      /// @brief Get en passant target square for current board state.
      /// @return Position of en passant target, or invalid position if no en passant available.
      /// @details Returns the square where an en passant capture could be executed.
      Position get_en_passant_target() const;

    private:
      /// @brief Internal value-type storage for piece data.
      /// @details PieceProperties is the authoritative source of truth for piece state.
      ///          OO interfaces are optional adapters for external consumers only.
      struct PieceRecord
      {
          PieceProperties properties;
      };

      std::vector<PieceRecord> pieces_; ///< Authoritative list using value-type storage
      std::array<int, 64>      square_to_piece_index_;
      uint64_t                 occupied_mask_ = 0;
      uint64_t                 white_occupied_mask_ = 0;
      uint64_t                 black_occupied_mask_ = 0;

      int square_index(const Position& pos) const;
      void rebuild_indexes();
      void add_piece_record(PieceType type, Color color, const Position& pos, bool has_moved, bool en_passant_vulnerable);
  };

} // namespace Chess

#endif // ICHESS_CCI_GRID
