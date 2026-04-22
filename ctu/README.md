# CTU: Chess Translation Unit

## Overview

The Chess Translation Unit (CTU) module handles translation between external chess notations and the engine's internal representations.

## Role: The "Adapter"

CTU acts as an adapter layer, allowing the chess engine to interface with different UIs (CLI, GUI), external AI engines, and standard chess notation systems.

## Core Responsibilities

1. **FEN Notation**
   - Convert FEN strings to internal Grid representation
   - Convert Grid representation back to FEN strings
   - Support all FEN fields (piece placement, active color, castling, en passant, halfmove, fullmove)

2. **Algebraic Notation**
   - Convert algebraic notation (e.g., "e2e4") to Move objects
   - Convert Move objects back to algebraic notation
   - Support long algebraic, short algebraic, and UCI formats

3. **Position Utilities**
   - Convert Position to algebraic coordinates (e.g., "a1", "h8")
   - Convert algebraic coordinates to Position

## Key Classes

### ChessTranslationUnit

Main class with public interface:

```cpp
class ChessTranslationUnit {
public:
    Grid fen_to_internal(const std::string& fen_string) const;
    std::string internal_to_fen(const Grid& grid) const;
    Move algebraic_to_move(const std::string& algebraic, const Grid& grid) const;
    std::string move_to_algebraic(const Move& move, const Grid& grid) const;
};
```

## Notation Formats

### FEN (Forsyth-Edwards Notation)

Format: `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1`

Fields:
1. Piece placement (from white's perspective)
2. Active color (w = white, b = black)
3. Castling availability (K = kingside, Q = queenside)
4. En passant target square (- if not available)
5. Halfmove clock (fifty-move rule)
6. Fullmove number (starts at 1)

### Algebraic Notation

Examples:
- `e2e4`: Move pawn from e2 to e4
- `e1g1`: Kingside castling (king moves from e1 to g1)
- `a7a8q`: Pawn promotion to queen

### UCI Format

Universal Chess Interface format:
- `e2e4`: Move from e2 to e4
- `e7e8q`: Promotion to queen

## Dependencies

- **CCI**: Uses Position, Move, Grid structures

## File Structure

```
ctu/
├── include/
│   └── translation_unit.h
├── src/
│   └── translation_unit.cpp
└── tests/
    ├── test_translation_unit.cpp
    └── test_fen_parsing.cpp
```

## Implementation Notes

- FEN parsing handles all valid FIDE legal positions
- Algebraic notation conversion includes ambiguity resolution
- Position coordinates follow standard chess notation (a-h, 1-8)
- Case-insensitive piece type parsing

## Error Handling

- Throws `std::invalid_argument` for malformed notations
- Validates position boundaries
- Verifies position legality when parsing FEN

## Future Enhancements

- Support for PGN (Portable Game Notation) parsing
- UCI protocol support
- Xboard protocol support
- Position comment and annotation support
