# CES: Chess Event System

## Overview

`chess_event_system` (CES) provides observer-style event publication for game and board lifecycle signals.

It allows other modules or app layers to subscribe to specific event types and react without tight coupling to CGH internals.

## Current API

### Event model

- `EventType` includes:
  - `MOVE_EXECUTED`
  - `PIECE_CAPTURED`
  - `POSITION_UPDATED`
  - `GAME_STARTED`
  - `GAME_ENDED`
  - `TURN_CHANGED`
  - `CASTLING_PERFORMED`
  - `EN_PASSANT_CAPTURE`
  - `PROMOTION_OCCURRED`
  - `CHECK_DETECTED`
  - `CHECKMATE_DETECTED`
  - `STALEMATE_DETECTED`
  - `DRAW_DETECTED`

- `EventData` contains:
  - `type`
  - `grid` pointer
  - `move`
  - `current_turn`
  - `timestamp`

- `EventListener`:
  - `std::function<void(const EventData&)>`

### `ChessEventSystem` (`event_system.h`)

- Subscription management:
  - `subscribe(EventType, EventListener)`
  - `unsubscribe(size_t)`
- Generic emission:
  - `emit_event(const EventData&)`
- Convenience emitters:
  - `emit_move_executed(...)`
  - `emit_piece_captured(...)`
  - `emit_position_updated(...)`
  - `emit_game_started(...)`
  - `emit_game_ended(...)`
  - `emit_turn_changed(...)`
  - `emit_check_detected(...)`
  - `emit_checkmate_detected(...)`
  - `emit_stalemate_detected(...)`
- Introspection:
  - `get_subscriber_count() const`
  - `get_subscriber_count(EventType) const`

## Module Layout

```
chess_event_system/
  include/
    event_system.h
  src/
    event_system.cpp
```

## Notes

- CES is used by CGH to broadcast move and game-state transitions.
- Subscription IDs are stable handles for deterministic unsubscription.
