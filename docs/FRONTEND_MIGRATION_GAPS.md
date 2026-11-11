# Frontend Migration Gaps

This report compares the migrated frontend with `backend/openapi.yaml`, the backend interfaces,
and the C++ domain interfaces. The local backend service and adapters added for Play are described
below; the existing C++ domain behavior was not changed.

## Features still unavailable

| Feature | Why it is disabled | Needed before enabling |
|---|---|---|
| Engine analysis | The domain has `ChessEngine`, but the backend has no analysis route and the frontend has no C++/WebAssembly bridge. | Expose a real analysis service and add a client for that service. |
| FEN position editor and PGN import/export | The backend game API does not expose import/export routes. The frontend no longer applies FEN/PGN locally. | Define and implement a supported browser-facing integration for position and notation operations. |

Play is now connected to the versioned game API. The frontend obtains game snapshots, move history,
and legal moves from the backend, and move commands are validated by the C++ domain. `chess.js`
and the local mock engine have been removed from the frontend.

## Backend/domain prerequisites and risks

- The new guest-session authenticator and game application are explicitly local-only, in-memory
  adapters. They are not appropriate for LAN or public deployment. Production still needs an
  established identity provider, secure session storage, and durable game storage.
- The local cookie omits `Secure` because the loopback development frontend uses plain HTTP. A
  production deployment must use HTTPS and a `Secure`, `HttpOnly`, `SameSite` cookie.
- Local session and idempotency maps have no expiry/retention policy, and games disappear at
  backend process exit.
- The local adapter rejects time-control requests; clock UI and authoritative clock handling are
  not implemented.
- The domain game handler does not preserve an en-passant-vulnerable flag when applying a
  double-square pawn move through its regular-move path. For example, after `e2e4`, its FEN omits
  the `e3` target, so the opposing side cannot make an otherwise legal en-passant capture. This
  domain behavior is documented but was not changed in this frontend/backend integration.
- `ChessGameHandler` is used as the rule/game authority. Its existing domain behavior should
  continue to be covered by the domain test suite; the frontend performs no chess-rule validation.
- The game API requires session cookies, CSRF tokens, idempotency keys, and strong version ETags
  for writes. The local frontend implements these for the same-origin development proxy.
- The backend exposes no game-analysis endpoint or streaming protocol. The C++ engine's presence
  in the domain does not make it callable from a browser.
- The backend has no routes for PGN/FEN import/export, analysis variations, or opening exploration.
  The frontend's old client-side chess.js and mock-analysis flows were removed rather than retained
  as misleading alternatives.

## Still available

Board/piece themes and display preferences remain local browser preferences. They do not require
backend/domain capabilities and are not synchronized across devices.
