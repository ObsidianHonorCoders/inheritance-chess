# CPL: Chess Pieces Logic

## Overview

`chess_pieces_logic` (CPL) generates raw piece-legal moves and attack maps from board geometry, independent of king-safety filtering.

CBM consumes CPL output to enforce full legality (check/checkmate/stalemate context).

## Current API

### `ChessPiecesLogic` (`pieces_logic.h`)

- `is_raw_logical_move(const Grid&, const Move&)`
- `list_raw_logical_moves(const Grid&, Position)`
- `list_raw_logical_moves_cached(const Grid&, Position, const PositionList&, const ColorList&)`
- `build_piece_cache(const Grid&, PositionList&, ColorList&)`
- `is_square_attacked(const Grid&, Position, Color)`

## Piece Implementations

Concrete piece classes are provided and wired through the shared factory path:

- `ChessPawn`
- `ChessKnight`
- `ChessBishop`
- `ChessRook`
- `ChessQueen`
- `ChessKing`

Implemented movement features include:

- Sliding-piece path blocking and captures
- Pawn double-step and en passant conditions
- Castling/promotion flag-aware generation paths
- Cached occupancy snapshots for repeated per-position queries

## Module Layout

```
chess_pieces_logic/
    include/
        pieces_logic.h
        chess_pawn.h
        chess_knight.h
        chess_bishop.h
        chess_rook.h
        chess_queen.h
        chess_king.h
    src/
        pieces_logic.cpp
        piece_factory.cpp
        chess_pawn.cpp
        chess_knight.cpp
        chess_bishop.cpp
        chess_rook.cpp
        chess_queen.cpp
        chess_king.cpp
    tests/
        test_pieces_logic.cpp
        test_pawn.cpp
        test_chess_pawn.cpp
        test_knight.cpp
        test_bishop.cpp
        test_rook.cpp
        test_queen.cpp
        test_king.cpp
        test_properties_integration.cpp
```

## Notes

- CPL intentionally does not decide checkmate/stalemate/draw outcomes.
- `is_square_attacked` is the primary attack-query seam used by CBM and CEI evaluation logic.
