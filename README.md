# Inheritance Chess

<img src="https://raw.githubusercontent.com/Calileus/inheritance-chess/a161e37a0641c0c9bf320bb5716651b8efa9d9cf/.github/inheritance-chess-icon.png" alt="inheritance-chess-icon" width="150" height="105">

## Release v0.0.0

See **[docs/RELEASE_NOTES.md](docs/RELEASE_NOTES.md)** for release notes, features, architecture, and testing information.

## Overview

Inheritance Chess is a modular C++ chess engine organized into focused components for shared board state, piece logic, rule enforcement, notation translation, event dispatch, orchestration, and engine search.

## Technical Specifications

- **Language**: C++20
- **Build System**: CMake (root project uses 3.20+, scripts use 3.28+)
- **Documentation**: Doxygen
- **Platform Support**: Windows, Linux, macOS
- **License**: Apache License 2.0

## Component Architecture

- `chess_common_interface` (CCI): shared types, `Grid`, `Move`, `Position`, apply/undo state
- `chess_pieces_logic` (CPL): raw piece move generation and attack detection
- `chess_board_manager` (CBM): legality filtering, check/mate/stalemate, draw checks
- `chess_translation_unit` (CTU): FEN/SAN/PGN/protocol translation
- `chess_event_system` (CES): event subscription and emission
- `chess_game_handler` (CGH): orchestration facade and game lifecycle control
- `chess_engine_interface` (CEI): search and evaluation engine

## Executable Entry Points

- `chess_game_handler/src/ichess_runner.cpp`: interactive runner executable (`ichess_runner`)
- `chess_game_handler/src/2p_game.cpp`: two-player console executable (`ichess_2p`)

## Build and Run

Primary scripts are under `cmake/`.

### Decoupled flow

```bash
cmake -P cmake/configure_and_build.cmake
cmake -P cmake/run_tests.cmake
cmake -P cmake/run_main_runner.cmake
cmake -P cmake/run_2p_game.cmake
```

### Legacy wrapper flow (compatibility)

```bash
cmake -P cmake/build_and_run_project.cmake
cmake -P cmake/build_2p_game_and_run.cmake
```

## Repository Structure (Top Level)

```
inheritance-chess/
	chess_common_interface/
	chess_pieces_logic/
	chess_board_manager/
	chess_translation_unit/
	chess_event_system/
	chess_game_handler/
	chess_engine_interface/
	cmake/
	docs/
	tests/
	CMakeLists.txt
	README.md
```

## Project Management

This project uses an engineering-focused Kanban workflow documented in `docs/kanban.md`.
