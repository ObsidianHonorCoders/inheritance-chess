# Inheritance Chess

<img src="https://raw.githubusercontent.com/Calileus/inheritance-chess/a3586f043568fc36b0fb3fe72d9ef45164db4b38/.github/inheritance-chess-icon.png" alt="inheritance-chess-icon" width="150" height="150">

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
- **Documentation**: Doxygen with Awesome CSS theme
- **Platform Support**: Windows, Linux
- **License**: Apache License 2.0

## Repository Structure

```
inheritance-chess/
├── include/                    # Public header files
│   ├── board.hpp              # Chess board interface and management
│   ├── common.hpp             # Common types and definitions
│   ├── piezas.hpp             # Base piece class definition
│   └── pawns.hpp              # Pawn-specific implementation
├── src/                       # Implementation files
│   ├── board.cpp              # Board implementation
│   ├── piezas.cpp             # Base piece class implementation
│   └── pawns.cpp              # Pawn implementation
├── doxy/                      # Documentation configuration
│   └── Doxyfile               # Doxygen configuration
├── build/                     # Build artifacts (generated)
├── doc/                       # Generated documentation output
├── main.cpp                   # Application entry point
├── CMakeLists.txt             # CMake build configuration
├── build_project_windows.cmd  # Windows build script
├── build_project_linux.sh     # Linux build script
├── .clang-format              # Code formatting standards
└── README.md                  # This file
```

### Directory Descriptions

- **include/**: Public API declarations defining the chess engine's interface
- **src/**: Implementation of core chess engine components
- **doxy/**: Documentation generation configuration and styling
- **build/**: Temporary directory containing compilation artifacts and build outputs
- **doc/**: Generated HTML and reference documentation
