# Inheritance Chess

<img src="https://raw.githubusercontent.com/Calileus/inheritance-chess/a161e37a0641c0c9bf320bb5716651b8efa9d9cf/.github/inheritance-chess-icon.png" alt="inheritance-chess-icon" width="150" height="105">

## Overview

Inheritance Chess is an enterprise-grade C++ chess engine that demonstrates object-oriented design principles and polymorphic architecture. Built with production-quality standards, this implementation leverages modern C++ practices to deliver a robust, maintainable, and extensible chess engine.

## Key Features

- **OOP Architecture**: Implements sophisticated inheritance hierarchies and polymorphism to model chess pieces and game mechanics
- **Type-Safe Design**: Type-safe operations and memory management
- **Extensible Framework**: Modular architecture enables seamless addition of new piece types and game rules
- **Standards Compliance**: Fully adheres to industry best practices and coding standards with integrated clang-format configuration
- **Comprehensive Documentation**: Complete Doxygen-generated API documentation with code examples

## Technical Specifications

- **Language**: C++ 17
- **Build System**: CMake 3.28+
- **Documentation**: Doxygen
- **Platform Support**: Windows and Linux
- **License**: Apache License 2.0

## Repository Structure

```
inheritance-chess/
├── include/
│   ├── board.hpp              # Chess board interface and management
│   ├── common.hpp             # Common types and definitions
│   ├── piezas.hpp             # Base piece class definition
│   └── pawns.hpp              # Pawn-specific implementation
├── src/
│   ├── board.cpp              # Board implementation
│   ├── piezas.cpp             # Base piece class implementation
│   └── pawns.cpp              # Pawn implementation
├── main.cpp                   # Application entry point
├── CMakeLists.txt             # CMake build configuration
├── build_project_windows.cmd  # Windows build script
├── build_project_linux.sh     # Linux build script
├── .clang-format              # Code formatting standards
└── README.md                  # This file
```

**Folder Descriptions**
- **include/**: Public declarations defining the chess engine's interface.
- **src/**: Implementation of core chess engine components.
- **build/**: Temporary directory containing compilation artifacts and build outputs (Not commited).
- **doc/**: Generated HTML and reference documentation (Not commited).

## Project Management

This project is managed using an engineering-focused Kanban board.
See [Kanban workflow](docs/kanban.md) for details on columns and flow.
