# CPL: Chess Pieces Logic

## Overview

The Chess Pieces Logic (CPL) module handles how individual pieces move in isolation, ignoring the "Check" rule. This module is the core move generation engine.

## Role: The "Physicist"

CPL calculates raw logical moves based on piece type and board geometry, without considering higher-level chess rules like check or checkmate.

## Core Responsibilities

1. **Move Generation**
   - Calculate all possible moves for each piece type
   - Implement piece-specific movement rules
   - Handle blocked squares and captures

2. **Square Attack Detection**
   - Determine if a square is attacked by a color
   - Support both piece type and color-based queries

3. **Special Move Logic**
   - En passant captures (handled with special flags)
   - Castling moves
   - Pawn promotion squares

## Key Classes

### ChessPiecesLogic

Main class with public interface:

```cpp
class ChessPiecesLogic {
public:
    bool is_raw_logical_move(const Grid& grid, const Move& move) const;
    std::vector<Move> list_raw_logical_moves(const Grid& grid, Position position) const;
    bool is_square_attacked(const Grid& grid, Position square, Color by_color) const;
private:
    std::vector<Move> get_pawn_moves(const Grid& grid, Position pos) const;
    std::vector<Move> get_knight_moves(const Grid& grid, Position pos) const;
    std::vector<Move> get_bishop_moves(const Grid& grid, Position pos) const;
    std::vector<Move> get_rook_moves(const Grid& grid, Position pos) const;
    std::vector<Move> get_queen_moves(const Grid& grid, Position pos) const;
    std::vector<Move> get_king_moves(const Grid& grid, Position pos) const;
};
```

## Piece Move Rules

### Pawn
- Moves forward one square (or two from starting position)
- Captures diagonally forward
- En passant capture (requires special flag)
- Promotion on reaching last rank

### Knight
- Moves in L-shape (2 squares in one direction, 1 in perpendicular)
- Not blocked by other pieces
- Can't move outside board

### Bishop
- Moves diagonally any number of squares
- Blocked by other pieces

### Rook
- Moves horizontally or vertically any number of squares
- Blocked by other pieces

### Queen
- Combines rook and bishop movement
- Moves horizontally, vertically, or diagonally

### King
- Moves one square in any direction
- Cannot move to attacked square (enforced by CBM)
- Castling is handled with special flags

## Dependencies

- **CCI**: Uses Position, Move, Grid structures

## File Structure

```
cpl/
├── include/
│   └── pieces_logic.h
├── src/
│   ├── pieces_logic.cpp
│   ├── pawn.cpp
│   ├── knight.cpp
│   ├── bishop.cpp
│   ├── rook.cpp
│   ├── queen.cpp
│   └── king.cpp
└── tests/
    ├── test_pieces_logic.cpp
    ├── test_pawn.cpp
    ├── test_knight.cpp
    ├── test_bishop.cpp
    ├── test_rook.cpp
    ├── test_queen.cpp
    └── test_king.cpp
```

## Implementation Strategy

- Use sliding move calculation for bishops, rooks, and queens
- Handle piece-specific rules in individual piece methods
- Use direction vectors for sliding pieces
- Include comprehensive test coverage for each piece type

## Performance Considerations

- Move generation should be < 1ms per position
- Consider bitboard representation for future optimization
- Cache frequently accessed moves during search

## Future Enhancements

- Bitboard representation for faster move generation
- Move ordering for better alpha-beta pruning
- Piece-specific move generation optimization
