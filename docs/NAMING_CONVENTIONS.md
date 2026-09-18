# Naming Conventions

This document describes naming conventions currently used in `inheritance-chess`.

## Files and folders
- Component folders: `chess_*`
- C++ source files: `snake_case.cpp`
- C++ headers: `snake_case.hpp`
- Test files: `test_*.cpp`
- CMake scripts: `snake_case.cmake`

## Symbols
- Namespaces: `snake_case`
- Classes/structs: `PascalCase`
- Functions: `snake_case`
- Variables: `snake_case`
- Constants/macros: `UPPER_SNAKE_CASE`

## Build names
- CMake options: `UPPER_SNAKE_CASE` (for example `BUILD_TESTING`)
- Component target names should remain stable (`cci`, `cpl`, `cbm`, `ctu`, `ces`, `cgh`, `cei`).
