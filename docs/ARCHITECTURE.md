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

## Repository boundaries
- `domain/core/` contains the existing C++ chess modules and console entry points.
- `backend/` contains the C++ HTTP adapter and provider interfaces; concrete authentication and durable game-application implementations remain to be supplied.
- `frontend/` is reserved for the future web client.

The proposed HTTP routes, security requirements, and deployment topology are defined in [SERVICE_CONTRACT.md](SERVICE_CONTRACT.md) and [backend/openapi.yaml](../backend/openapi.yaml).

## Runtime orchestration
- `ichess_runner` and `ichess_2p` both orchestrate gameplay through `domain/core/chess_game_handler`.
- Rule validation and legal move control come from board/pieces modules.
- Translation, eventing, and engine behavior are composed through module interfaces.

## Build boundaries
- Each module builds as its own target.
- Root executable targets link all module libraries.
- Centralized tests are assembled under root `tests/` with explicit file lists.

## Governance and history
- Governance policies are inherited from organization `.github`.
- Historical notes are maintained only in `CHANGELOG.md`.
