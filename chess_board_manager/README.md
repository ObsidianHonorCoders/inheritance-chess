# CBM: Chess Board Manager

## Overview

`chess_board_manager` (CBM) enforces high-level chess rules on top of raw move generation.

It validates moves against king safety, resolves check/checkmate/stalemate status, and provides legal-move lists used by CGH and CEI.

## Current API

`ChessBoardManager` (`board_manager.h`):

- `is_white_king_in_check(const Grid&)`
- `is_black_king_in_check(const Grid&)`
- `is_king_in_check(const Grid&, Color)`
- `validate_move(const Grid&, const Move&)`
- `get_legal_moves(const Grid&)`
- `is_checkmate(const Grid&, Color)`
- `is_stalemate(const Grid&, Color)`
- `is_draw(const Grid&, Color)`

## Draw Logic Currently Implemented in CBM

- Fifty-move rule (`halfmove_clock >= 100`)
- Insufficient material heuristics:
    - K vs K
    - K + minor vs K
    - K + N + N vs K (same side)
    - K + B vs K + B when bishops are on same-colored squares

Note: threefold repetition state tracking is handled at orchestration level in CGH using canonical position-history keys.

## Module Layout

```
chess_board_manager/
    include/
        board_manager.h
    src/
        board_manager.cpp
    tests/
        test_board_manager.cpp
        test_validation.cpp
```

## Notes

- CBM uses CPL for raw move generation/attack checks and applies rule-level filtering.
- `get_legal_moves()` is the central move list used for mate/stalemate determination.
