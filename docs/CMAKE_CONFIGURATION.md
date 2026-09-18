# CMake Configuration

This document describes the current CMake workflow for `inheritance-chess`.

## Minimum requirements
- Root project CMake minimum: 3.20
- Script workflows in `cmake/`: 3.28
- C++ standard: C++20

## Presets
Defined in `CMakePresets.json`:
- `dev` → Debug configure (`build/dev`)
- `ci` → Release configure (`build/ci`)
- `dev-build`, `ci-build`
- `dev-test`, `ci-test`

## Standard workflow
```bash
cmake --preset dev
cmake --build --preset dev-build
ctest --preset dev-test --output-on-failure
```

## Key cache options
- `BUILD_TESTING` (`ON`/`OFF`): include centralized tests.
- `EXE_NAME`: primary runner target name (defaults to `ichess_runner`).
- `OHC_ENABLE_WERROR` (`ON`/`OFF`): treat warnings as errors.

## Script workflows
Primary scripts in `cmake/`:
- `configure_and_build.cmake`
- `run_tests.cmake`
- `run_main_runner.cmake`
- `run_2p_game.cmake`

Legacy wrappers in `cmake/` remain for compatibility:
- `build_and_run_project.cmake`
- `build_2p_game_and_run.cmake`
