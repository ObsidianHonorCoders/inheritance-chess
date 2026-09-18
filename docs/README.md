# Inheritance Chess Documentation

This documentation describes the current state of the repository.

## Current Scope
- Modular C++20 chess engine with seven focused components.
- Two executables:
  - `ichess_runner` (interactive)
  - `ichess_2p` (local two-player)
- Centralized test execution through root `tests/`.

## Build and Test
- Configure/build with presets:
  - `cmake --preset dev`
  - `cmake --build --preset dev-build`
- Run tests:
  - `ctest --preset dev-test --output-on-failure`

## Component Docs
- `chess_common_interface/README.md`
- `chess_pieces_logic/README.md`
- `chess_board_manager/README.md`
- `chess_translation_unit/README.md`
- `chess_event_system/README.md`
- `chess_game_handler/README.md`
- `chess_engine_interface/README.md`

## Project Workflow
- Kanban and planning conventions: `docs/kanban.md`

## Docs Set
- `ARCHITECTURE.md`
- `CMAKE_CONFIGURATION.md`
- `TESTING.md`
- `NAMING_CONVENTIONS.md`
- `TROUBLESHOOTING.md`

## History
- Historical status notes are kept only in `CHANGELOG.md`.
