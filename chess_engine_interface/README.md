# CEI: Chess Engine Interface

## Overview

`chess_engine_interface` (CEI) provides engine-side search and evaluation for move selection.

## Current API

### Core structures

- `EvaluationResult`:
  - `score`, `best_move`, `depth`, `nodes_searched`, `time_used`
- `SearchLimits`:
  - `max_depth`
  - `max_time`
  - `max_nodes`
  - `infinite`

### `ChessEngine` (`chess_engine.h`)

- `find_best_move(const Grid&, const SearchLimits&)`
- `evaluate_position(const Grid&, Color)`
- `evaluate_material(const Grid&)`
- `get_legal_moves(const Grid&)`
- `is_draw(const Grid&)`
- `set_difficulty(int)`
- `get_statistics() const`
- `reset_statistics()`

## Search/Evaluation Features

- Minimax with alpha-beta pruning
- Quiescence search extension
- Zobrist hashing and transposition-table probing/storing
- Time/node/depth stop conditions via `SearchLimits`
- Runtime search statistics accumulation

## Module Layout

```
chess_engine_interface/
  include/
    chess_engine.h
  src/
    chess_engine.cpp
  tests/
    test_chess_engine.cpp
```

## Notes

- CEI uses project move-generation and rule layers rather than duplicating legality logic.
- The API is suitable for direct integration and protocol-driven play (through CTU protocol adapter paths).
