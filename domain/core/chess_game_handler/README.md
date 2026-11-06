# CGH: Chess Game Handler

## Overview

`chess_game_handler` (CGH) is the orchestration facade for game lifecycle and module coordination.

It owns module instances, tracks session metadata, updates game state after moves, emits events, and exposes a stable API for app entry points.

## Current API

### `GameSession`

- `game_id`
- `move_history`
- `white_time`, `black_time`
- `state`

### `ChessGameHandler` (`game_handler.h`)

- Lifecycle:
  - `start_new_game()`
  - `load_game_from_fen(const std::string&)`
- Move execution:
  - `make_move(const std::string&)`
  - `make_move(const Move&)`
- Position and rules:
  - `get_current_fen()`
  - `get_legal_moves()` and `get_legal_moves(const Grid&)`
  - `get_game_state()`
  - `get_current_grid()`
- Timing:
  - `start_timer(Color)`
  - `stop_timer(Color)`
  - `get_time_remaining(Color)`
- UI/event seams:
  - `display_board()`
  - `get_event_system()`

## Draw and State Handling

CGH computes final game state through CBM + local orchestration checks:

- Checkmate
- Stalemate
- Draw by insufficient material / fifty-move rule (via CBM)
- Draw by threefold repetition (via CGH position-history map)

Threefold repetition uses canonical keys built from the first four FEN fields and is recorded after position transitions.

## Module Layout

```
chess_game_handler/
  include/
    game_handler.h
  src/
    game_handler.cpp
  tests/
    test_game_handler.cpp
    test_integration.cpp
```

## Notes

- CGH is designed as the central integration boundary for CLI/app flows.
- Current implementation is single-thread oriented and does not provide internal synchronization primitives.
