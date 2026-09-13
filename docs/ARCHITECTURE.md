# Architecture

This document describes the current architecture of `inheritance-chess`.

## System shape
- Modular C++20 chess engine repository.
- Seven component libraries:
  - `chess_common_interface` (CCI)
  - `chess_pieces_logic` (CPL)
  - `chess_board_manager` (CBM)
  - `chess_translation_unit` (CTU)
  - `chess_event_system` (CES)
  - `chess_game_handler` (CGH)
  - `chess_engine_interface` (CEI)
- Two executables:
  - `ichess_runner`
  - `ichess_2p`

## Approved structure exception
This repository intentionally keeps a multi-module layout instead of a single root `include/` + `src/` project layout.

## Runtime orchestration
- `ichess_runner` and `ichess_2p` both orchestrate gameplay through `chess_game_handler`.
- Rule validation and legal move control come from board/pieces modules.
- Translation, eventing, and engine behavior are composed through module interfaces.

## Build boundaries
- Each module builds as its own target.
- Root executable targets link all module libraries.
- Centralized tests are assembled under root `tests/` with explicit file lists.

## Governance and history
- Governance policies are inherited from organization `.github`.
- Historical notes are maintained only in `CHANGELOG.md`.
