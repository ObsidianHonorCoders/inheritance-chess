# Changelog

## Unreleased
- Moved chess components under `domain/core/` and added the `backend/` HTTP service (cpp-httplib, OpenAPI contract in `backend/openapi.yaml`).
- Added `ICHESS_BUILD_BACKEND` option (default ON) to skip the backend and its network-fetched dependencies.
- Fixed `GET /api/v1/games/{id}/moves?limit=0` to return `400 invalid_query`.

## 2026-09-13
- Standardized root build/test contract to `BUILD_TESTING` with compatibility shims.
- Removed local governance duplicates; use org `.github` policies.
- Added repository hygiene baselines (`.editorconfig`, `.pre-commit-config.yaml`).

## 2026-08-12
- Initial modular chess engine baseline with dual executables and centralized tests.
