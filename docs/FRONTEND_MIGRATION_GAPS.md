# Frontend Migration Gaps

This report compares the migrated frontend with `backend/openapi.yaml`, the backend interfaces,
and the C++ domain interfaces. Backend and domain code were not changed as part of this migration.

## Disabled frontend features

| Feature | Why it is disabled | Needed before enabling |
|---|---|---|
| Play/game screen | The API contract defines game operations, but the repository has no concrete `IGameApplication` implementation or usable game service. The frontend's current game store runs chess.js locally and does not call the game API. | Implement and run the game application adapter, then integrate the frontend with the versioned game endpoints and server snapshots. |
| Engine analysis | The frontend currently uses `mockEngineApi`. The domain has `ChessEngine`, but the backend has no analysis route and the frontend has no C++/WebAssembly bridge. | Expose a real analysis service and replace the mock with a client for that service. |
| FEN position editor and PGN import/export | Domain translation supports FEN and PGN, but the frontend currently handles these locally; there is no frontend-to-domain bridge or backend endpoint for these operations. | Define and implement a supported browser-facing integration for position and notation operations. |

Direct visits to `/play` and `/analysis` show an unavailable screen; those links are omitted from
the main navigation. Their existing component code is retained in `src/` but is not mounted.

## Backend/domain prerequisites and risks

- `IRequestAuthenticator` has no implementation. The backend README requires a validated
  server-side session and CSRF/Origin verification for writes. Game features must not be enabled
  with an identity-header bypass or before this security boundary is implemented.
- `IGameApplication` has no implementation. Its contract requires authorization, durable game
  state, version checks, and idempotent writes; the HTTP adapter alone cannot provide these.
- The frontend currently uses chess.js for local rules, while the authoritative rules live in the
  C++ domain. Keeping the local store active after backend integration could create divergent game
  state; server snapshots and legal-move results should be authoritative.
- The game API requires session cookies, CSRF tokens, idempotency keys, and strong version ETags
  for writes. The current frontend does not implement this request lifecycle.
- The backend exposes no game-analysis endpoint or streaming protocol. The C++ engine's presence
  in the domain does not make it callable from a browser.
- The backend has no routes for PGN/FEN import/export, analysis variations, or opening exploration.
  The domain supports notation conversion, but frontend use needs a defined integration boundary.

## Still available

Board/piece themes, display preferences, animation, and sounds remain local browser preferences.
They do not require backend/domain capabilities and are not synchronized across devices.
