# Testing

This document describes the current test setup for `inheritance-chess`.

## Framework
- GoogleTest via CMake FetchContent
- Test registration via `gtest_discover_tests`
- Execution via CTest presets

## Current test model
- Root `tests/CMakeLists.txt` defines one centralized test executable:
  - `test_${EXE_NAME}`
- Test sources are explicitly listed from each module test folder.

## Run tests
```bash
cmake --preset dev
cmake --build --preset dev-build
ctest --preset dev-test --output-on-failure
```

## Add tests
1. Add test source in the relevant module `tests/` folder.
2. Register the file explicitly in root `tests/CMakeLists.txt` source lists.
3. Reconfigure and run CTest preset.
