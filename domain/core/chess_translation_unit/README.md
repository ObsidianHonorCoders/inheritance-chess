# CTU: Chess Translation Unit

## Overview

`chess_translation_unit` (CTU) translates between external chess notations/protocol commands and internal CCI structures.

## Current API

### `ChessTranslationUnit` (`translation_unit.h`)

- FEN:
  - `fen_to_internal(const std::string&)`
  - `internal_to_fen(const Grid&)`
- Coordinate/algebraic:
  - `algebraic_to_move(const std::string&, const Grid&)`
  - `move_to_algebraic(const Move&, const Grid&)`
- SAN:
  - `san_to_move(const std::string&, const Grid&)`
  - `move_to_san(const Move&, const Grid&)`
- Board rendering:
  - `render_board_ascii(const Grid&)`
- PGN move text:
  - `moves_to_pgn(const std::vector<Move>&, const Grid&, const std::string& result = "*")`
  - `pgn_to_moves(const std::string&, const Grid&)`
- Rich PGN parse/serialize:
  - `parse_pgn_game(const std::string&, const Grid&, const PGNParseOptions&)`
  - `pgn_game_to_string(const PGNGame&, const Grid&, const PGNSerializeOptions&)`

### Protocol Adapter

`ChessProtocolAdapter` exposes command handlers for:

- UCI (`handle_uci_command`)
- XBoard (`handle_xboard_command`)

Using:

- `ProtocolAdapterState`
- `ProtocolCommandResult`

## PGN Data Structures

- `PGNGame`
- `PGNMoveNode`
- `PGNVariation`
- `PGNParseOptions`
- `PGNSerializeOptions`

These represent tags, comments, annotations, recursive variations, and tolerant parsing behavior.

## Module Layout

```
chess_translation_unit/
  include/
    translation_unit.h
  src/
    translation_unit.cpp
  tests/
    test_translation_unit.cpp
    test_fen_parser.cpp
    test_algebraic.cpp
    test_san.cpp
    test_pgn.cpp
    test_protocol_adapter.cpp
```

## Notes

- CTU is the module boundary for external notation/protocol interoperability.
- CGH and executable entry points depend on CTU for text I/O and command translation.
