# CCI: Chess Common Interface

## Overview

`chess_common_interface` (CCI) defines the shared data model used across the project: colors, piece types, moves, positions, board state, and in-place apply/undo support.

CCI is intentionally logic-light and provides the canonical board representation used by CPL, CBM, CTU, CGH, and CEI.

## Current API Surface

### `chess_types.h`

- `Color`, `PieceType`, `SpecialFlags`
- `GameState` including:
    - `DRAW_INSUFFICIENT_MATERIAL`
    - `DRAW_THREEFOLD_REPETITION`
    - `DRAW_FIFTY_MOVE_RULE`
- `GameFlags` for castling rights and move counters

### `position.h`

- `Position { file, rank }`
- Validation and comparison operators
- Conversion helpers:
    - `position_to_algebraic(const Position&)`
    - `algebraic_to_position(const std::string&)`

### `move.h`

- `Move { start_pos, end_pos, flags, promotion_piece }`
- Helpers:
    - `is_promotion()`
    - `is_castling()`
    - `is_en_passant()`

### `grid.h`

`Grid` is the authoritative board-state container and supports both value-type piece access and optional OO adapters.

Key capabilities:

- Board/state operations:
    - `initialize_standard_position()`
    - `get_piece()`, `set_piece()`, `clear_square()`, `is_occupied()`
    - `get_all_piece_properties()` (authoritative)
    - `switch_turn()`
- Search-oriented operations:
    - `clone()`
    - `apply_move_inplace(const Move&) -> UndoRecord`
    - `undo_move(const Move&, const UndoRecord&)`
- Optional OO bridge:
    - `ChessPiece` polymorphic interface
    - `PieceList`, `get_all_pieces()`, `get_piece_at()`
    - `create_piece(PieceType, Color, Position)` factory declaration

## Module Layout

```
chess_common_interface/
    include/
        chess_types.h
        position.h
        move.h
        grid.h
    src/
        chess_types.cpp
        position.cpp
        move.cpp
        grid.cpp
    tests/
        test_chess_types.cpp
        test_position.cpp
        test_move.cpp
        test_grid.cpp
        test_grid_adapted.cpp
        test_piece_factory.cpp
        test_main.cpp
```

## Notes

- `Grid` copy is intentionally disabled; use move or `clone()`.
- CCI remains the single source of truth for board state and shared chess types.
