# Service Contract

This document defines the first browser-facing service boundary. The machine-readable HTTP contract is [backend/openapi.yaml](../backend/openapi.yaml). The OpenAPI document is authoritative for paths, JSON shapes, and status codes; this document defines required behavior, security controls, and deployment architecture.

## Scope and Defaults

- API base path: `/api/v1`.
- JSON over HTTPS; one public origin serves the frontend and proxies `/api/*` to the backend.
- Use URL-safe game IDs generated with at least 128 bits of cryptographic entropy; IDs are not credentials. Do not expose the current time-derived core `game_id` as an access token.
- The backend is authoritative for game state, move legality, clocks, and outcomes. The browser never decides whether a move is legal.
- Version 1 supports authenticated, two-seat standard chess. The creator occupies White; the first eligible second participant joins as Black. Spectators and matchmaking are out of scope.
- Initial FEN is optional and defaults to the standard position. Only standard chess is supported; variants are not implied.
- Game mutations after creation require the latest game ETag in `If-Match`. A successful mutation returns the complete new snapshot and its new ETag. Creation instead requires an idempotency key.
- Every state-changing request requires an `Idempotency-Key`. Reuse it only to retry the same logical request after an uncertain network result; reusing it with a different payload is a conflict.
- Idempotency records are scoped to the authenticated principal and route, retained for a documented bounded period, and checked before applying a fresh version precondition on a completed retry. Reauthorize the request before replaying a stored response.
- Moves use lowercase UCI coordinates such as `e2e4` or `e7e8q`; SAN is presentation/history data, not the command format.
- All errors use `application/problem+json` (RFC 9457) with a stable application `code`. Do not return stack traces, SQL details, filesystem paths, or internal C++ diagnostics.

## HTTP Behavior

| Operation | Purpose | Important behavior |
|---|---|---|
| `GET /api/v1/health/live` | Process liveness | No dependency or deployment details in the response. Keep it private to the proxy/health-check network. |
| `GET /api/v1/health/ready` | Dependency readiness | Returns `503` if required storage is unavailable; keep private to the proxy/health-check network. |
| `GET /api/v1/session` | Session/CSRF bootstrap | Requires an established session; returns a session-bound CSRF token without exposing the internal principal ID. Send `Cache-Control: no-store`. |
| `POST /api/v1/games` | Create game | Requires `Idempotency-Key`; retries with the same key and same request return the original result. Creator is White. |
| `GET /api/v1/games/{gameId}` | Read snapshot | Requires a participant session; use `ETag` for subsequent writes. |
| `POST /api/v1/games/{gameId}` | Join as Black | Requires `If-Match` and `Idempotency-Key`. Only an unoccupied seat can be claimed. |
| `GET /api/v1/games/{gameId}/legal-moves` | Read legal moves | Returns legal UCI/SAN pairs with the associated version. Move submission always revalidates. |
| `GET /api/v1/games/{gameId}/moves` | Read move history | Cursor by ply; page size is bounded. |
| `POST /api/v1/games/{gameId}/moves` | Submit move | Requires `If-Match` and `Idempotency-Key`; validates seat, turn, game status, UCI shape, and legality atomically. |
| `POST /api/v1/games/{gameId}/resign` | Resign | Requires `If-Match` and `Idempotency-Key`; acts only for the authenticated caller's seat. |
| `POST /api/v1/games/{gameId}/draw-offers` | Offer draw | Requires `If-Match` and `Idempotency-Key`; acts only for the authenticated caller's seat. |
| `POST /api/v1/games/{gameId}/draw-offers/accept` | Accept draw | Requires `If-Match` and `Idempotency-Key`, plus an opponent offer. |

Error semantics: `400` malformed request; `401` no valid session; `403` CSRF or origin validation failed; `404` nonexistent or non-visible game; `409` game/seat state disallows the operation or an idempotency key is reused with a different payload; `412` stale `If-Match`; `413` body exceeds the configured limit; `422` syntactically valid but invalid FEN or illegal move; `428` required `If-Match` is missing; `429` request limit exceeded, with `Retry-After` when available. Do not distinguish an unknown game from another user's game in responses.

`GameSnapshot.version` is monotonically incremented after every successful state mutation. The strong ETag represents that version (for example, `"7"`). Clients must fetch the latest snapshot after `412`; they must not retry a move against a newer version without user or application-level reconciliation.

## Identity and Authorization

- Every game endpoint requires an authenticated principal. The backend maps the verified principal to a seat; possession or knowledge of a game ID is never authorization.
- For a same-origin browser, use an opaque server-side session in a `__Host-ichess_session` cookie with `Secure`, `HttpOnly`, `Path=/`, no `Domain`, and an appropriate `SameSite` policy. Rotate the session on authentication and privilege changes; expire and revoke it server-side.
- Every state-changing cookie-authenticated request requires a CSRF token in `X-CSRF-Token`, obtained from `GET /api/v1/session` and checked against the session. Do not cache the bootstrap response. Also validate `Origin` against the configured application origin. `SameSite` is defense in depth, not the only CSRF control.
- For a private preview, Cloudflare Access can add an identity gate, but the backend must still validate trusted identity assertions and authorize each game operation. Never trust a client-supplied identity header. Strip identity headers at the proxy, accept assertions only from the trusted connector path, and validate signature, issuer, audience, and expiry.
- Local username/password registration and login are supported through server-side accounts with salted PBKDF2-HMAC-SHA256 password hashes; registration can be disabled by the operator. Password reset, email verification, and MFA are not currently provided, so define account-recovery and abuse policies before broad public onboarding. Social login is optional and can later map a verified external identity to the same server-side session interface. Never put passwords, long-lived credentials, or provider secrets in frontend code or browser storage.
- The creator can read and mutate their game while seated. The second participant can access it only after joining. Each mutation additionally checks the caller's seat, turn where applicable, and ongoing game state.
- Return `404` for games the caller cannot see to reduce enumeration; the high-entropy identifier is not authorization.

## Input and Browser Controls

- Accept only expected JSON content types and documented fields. Reject malformed JSON, unknown fields on mutation DTOs, invalid enums, oversized bodies, invalid encodings, and out-of-range numbers before domain calls.
- Bound FEN length and validate it through the core parser. Bound display names and normalize them as plain text. Escape user-provided text on output; never render it as HTML.
- Apply request-size, connection, and execution-time limits at both proxy and backend. Keep the backend listener private and do not expose an alternate public port.
- Same-origin deployment is the default; do not enable wildcard CORS. If a separate frontend origin is later required, allow only explicit origins and credential rules, and retain CSRF protection.
- Return `Cache-Control: private, no-store` for session and game data. Do not cache authenticated API responses at the browser, reverse proxy, or Cloudflare edge.
- Serve the frontend with a restrictive Content Security Policy, `frame-ancestors 'none'`, `X-Content-Type-Options: nosniff`, and a deliberate `Referrer-Policy`. Avoid inline scripts and third-party scripts that can access the application origin.

## Abuse Resistance and Data Protection

- Rate-limit by authenticated principal and, as a secondary signal, source IP. Apply tighter budgets to game creation, joins, and expensive operations. Enforce limits in the backend even when Cloudflare rate limiting is configured.
- If engine endpoints are added, require strict maximum depth, nodes, and wall-clock time; limit concurrent searches per principal and globally; make cancellation possible. Engine search is not part of this v1 contract.
- Use parameterized database operations, least-privilege database credentials, transaction boundaries, and schema constraints. Never build SQL from request strings.
- Store only necessary account and game data. Protect database files, backups, and secrets with host-level permissions and encryption appropriate to the deployment. Define retention and deletion behavior before adding account data.
- Do not log cookies, CSRF tokens, authorization headers, Access tokens, or full request bodies. Log request ID, principal identifier in a privacy-conscious form, game ID, action, result code, latency, and version conflict outcome. Restrict and rotate logs.
- Keep secrets out of source, frontend bundles, images, and checked-in configuration. Use deployment secret injection and rotate credentials. Run containers as non-root, pin and review dependencies, and scan dependencies/images in CI.

## Efficient Runtime Architecture

```text
Browser
  -> Cloudflare edge (TLS, optional Access, WAF/rate controls)
  -> cloudflared outbound tunnel
  -> private reverse proxy
       /       -> static frontend
       /api/*  -> C++ API process
  -> durable game store
```

- The Tunnel is ingress transport, not application authentication, authorization, or storage. Keep origin ports closed to the public Internet; bind the API only to loopback or a private container network. Route health probes privately.
- Keep `frontend/` and `backend/` deployable independently. The frontend is static assets. The C++ API links the `domain/core` CMake targets directly and converts HTTP DTOs to core types; do not expose C++ object layouts as JSON.
- Treat the database as durable source of truth. Persist game ID, participant principals, initial FEN, ordered UCI move log, status, clocks/time control, version, draw-offer state, and timestamps. Use a transaction to check the expected version, apply one move, append history, update the snapshot/version, and commit as one unit.
- The current core handler and event system are not thread-safe. Do not share a mutable `ChessGameHandler` concurrently across requests. Serialize writes per game and enforce the version check in storage so multiple API processes cannot accept conflicting moves.
- To restore a game, create a handler from its initial position and replay the ordered move log. Restoring from only the latest FEN loses repetition history. A bounded cache of reconstructed active games is optional; it must be disposable and never be the sole source of truth.
- Start with one API instance and SQLite on the same host if operational simplicity is the priority. Use PostgreSQL when concurrent writers, multiple API instances, or stronger database operations are required. The API contract is independent of that choice.
- Use ordinary request/response for commands and snapshots in v1. Add authenticated SSE for server-to-browser updates only when live play requires it; keep moves as REST writes. Include reconnect cursors and reauthorize the game on stream start. Do not add a message broker until deployment scale requires it.

## Verification Requirements

Before public deployment, tests must cover authentication and seat authorization, CSRF and origin rejection, stale-version races, duplicate create retries, illegal move rejection, terminal-state behavior, input size limits, error redaction, and restoration from initial FEN plus full move history. Deployment checks must confirm the API cannot be reached except through the private proxy/tunnel path and that secrets are absent from frontend artifacts and logs.

The identity provider, account onboarding, invite/link workflow, clock policy, and public spectator policy remain product decisions. They must not weaken the authorization rules above.