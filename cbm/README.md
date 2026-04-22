/// @file      README.md
/// @namespace Chess
/// @brief     Chess Board Manager (CBM) Module Documentation.
/// @author    Calileus
/// @date      2026-01-22
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess

# CBM: Chess Board Manager

## Overview

The Chess Board Manager (CBM) module is responsible for managing the global state of the board and enforcing the higher-level rules of chess.

## Role: The "Referee"

CBM acts as the arbiter of the game, checking move legality, detecting end-game states, and verifying that moves don't leave the king in check.

## Core Responsibilities

1. **Board State Management**
   - Manage current game board
   - Maintain validation grid for "what-if" scenarios
   - Track game state changes

2. **Move Validation**
   - Verify that a move doesn't leave the king in check
   - Check move legality using CPL's move generation
   - Update board state after moves

3. **End-Game Detection**
   - **Checkmate**: King in check with no legal moves
   - **Stalemate**: King not in check but no legal moves
   - **Draw**: Insufficient material, threefold repetition, fifty-move rule

## Key Classes

### ChessBoardManager

Main class with public interface:

```cpp
class ChessBoardManager {
public:
    bool is_white_king_in_check(const Grid& grid) const;
    bool is_black_king_in_check(const Grid& grid) const;
    bool is_draw(const Grid& grid, Color color_turn) const;
    bool validate_move(const Grid& grid, const Move& move) const;
    bool is_checkmate(const Grid& grid, Color color) const;
    bool is_stalemate(const Grid& grid, Color color) const;
private:
    Grid current_board_;
    Grid validation_grid_;
};
```

## Dependencies

- **CCI**: Uses Position, Move, Grid structures
- **CPL**: Depends on piece move logic for validation

## File Structure

```
cbm/
├── include/
│   └── board_manager.h
├── src/
│   └── board_manager.cpp
└── tests/
    ├── test_board_manager.cpp
    └── test_validation.cpp
```

## Implementation Notes

- Uses CPL's `is_square_attacked()` to check if king is in check
- Maintains validation grid for "what-if" move scenarios
- Separates rule enforcement from move generation logic

## Future Enhancements

- Move history tracking for draw detection
- Zobrist hashing for position caching
- Transposition table support
