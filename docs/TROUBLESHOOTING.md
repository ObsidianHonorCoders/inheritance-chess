# Troubleshooting

This guide covers current common issues in `inheritance-chess`.

## Configure fails
- Verify CMake/toolchain availability.
- Run:
```bash
cmake --preset dev
```

## Build fails
- Build with diagnostics:
```bash
cmake --build --preset dev-build --verbose
```
- If stale artifacts are suspected, remove `build/dev` and reconfigure.

## Tests missing or not running
- Confirm `BUILD_TESTING=ON` in presets or configure arguments.
- Reconfigure then run:
```bash
ctest --preset dev-test --output-on-failure
```

## Script entrypoint confusion
- Preferred decoupled scripts are under `cmake/` (`configure_and_build`, `run_tests`, `run_main_runner`, `run_2p_game`).
- Legacy wrappers remain for compatibility and call into the same script flow.
