# Inheritance Chess Frontend

The React/Vite web frontend lives in this directory. It is currently a standalone frontend
project; it is not part of the CMake build.

## Development

```bash
npm ci
npm run dev
npm run typecheck
npm run build
```

Requires Node.js 18.18 or newer.

## Current availability

Settings (board and piece themes, sound, animations, and coordinates) work locally in the browser.
Play and analysis routes are intentionally unavailable:

- The backend defines the versioned game API, but concrete game-application and session
  authentication adapters are not present.
- The domain engine is a C++ library and is not exposed through a browser-callable analysis API.
- The existing local chess.js game and mock engine are not used to present Play or Analysis as
  backend-backed features.

See [the migration gaps report](../docs/FRONTEND_MIGRATION_GAPS.md) for missing capabilities and
integration risks.
