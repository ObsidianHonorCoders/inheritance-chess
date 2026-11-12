# Backend Adapter

`ichess_backend` is the C++ HTTP/JSON adapter for the contract in [openapi.yaml](openapi.yaml). It links the existing game-handler and translation-unit targets, but keeps transport DTOs separate from chess-core types.

## Build Boundary

- `include/http_api.h` registers versioned API routes on a `cpp-httplib` server.
- `IRequestAuthenticator` must validate the server-side session and, on writes, CSRF token and allowed origin. The interface intentionally has no permissive default implementation.
- `IGameApplication` owns authorization, versioned/idempotent operations, and repository transactions. Its implementation must enforce game-seat ownership and persist initial FEN plus the ordered move log.
- `api_types.h` contains transport-neutral API DTOs and safe application errors.

`ichess_service` composes SQLite gameplay and concrete server-side session authentication for a restricted private preview. It binds only to `127.0.0.1`. There is no identity-header bypass, public session-minting route, or public account login flow. See [DEPLOYMENT.md](DEPLOYMENT.md) before configuring ingress. This is not a production-readiness certification.

## SQLite Application Foundation

`SqliteDatabase` owns a full-mutex SQLite connection with foreign keys, defensive mode, untrusted schema, WAL, full synchronization, and a one-second busy timeout. Schema version 1 includes games, ordered moves, and idempotency records. Startup validates table columns, keys, foreign keys, and constraint behavior; unsupported or malformed schemas fail closed without repair. Live readiness checks game storage and the service's revocation store. DDL changes require reopening for revalidation. The database must outlive every `SqliteGameApplication` referencing it. Keep the database on local durable storage, not a network filesystem.

`SqliteGameApplication` implements all untimed game operations: create, snapshot, join, legal moves, paginated history, move submission, resignation, and draw offer/acceptance:

- Mutations are atomic with their idempotency records. A retry scoped to the same principal and operation/resource returns the stored original snapshot before checking the fresh version precondition. A different payload/version under the same key returns `409`.
- Idempotency results remain eligible for replay for 24 hours. An expired key may create a new game. Requests clean up bounded batches, and the service runs a 60-second maintenance pass deleting at most 256 expired records each time. This bounds cleanup work per pass while draining retained rows during inactive periods.
- Retries recheck current game visibility before replay. Unknown and non-visible games both return `404`. Storage failures return redacted errors rather than being presented as missing games.
- Public IDs use 128 bits from the OS random source: BCrypt on Windows and `/dev/urandom` on Unix-like systems. IDs are not credentials.
- Handlers are reconstructed from initial FEN and contiguous ordered moves, retaining repetition history. A read transaction keeps metadata and moves consistent. Wrappers sharing a database synchronize on its connection mutex; independent connections serialize mutations through SQLite write transactions. Each mutation rechecks participant, seat, turn, status, and expected version. Competing writes cannot both commit at the same version.
- Initial FEN must round-trip in canonical form and pass king, pawn, counter, castling, and en-passant consistency checks. These checks are not a proof of historical reachability. Terminal positions do not advertise playable moves. History is capped at 4096 plies; longer games return a controlled conflict rather than unbounded replay work.

Timed creation still returns `422` until restart-safe clock behavior is implemented. Do not present timed play in a client. Live readiness is a lightweight read/schema probe, not proof of available disk capacity, successful future writes, or end-to-end deployment health.

## Private Preview Runtime

`SessionAuthenticator` provides bounded in-memory, expiring sessions; random 256-bit opaque credentials and CSRF tokens; strict cookie parsing; exact HTTPS Origin checks; and secure cookie attributes. Local accounts use case-insensitive ASCII usernames and per-account salted PBKDF2-HMAC-SHA256 password hashes (600,000 iterations); registration/login issue the same server-side session and logout revokes it. Operators can close registration with `ICHESS_ALLOW_REGISTRATION=false`. Login and registration have separate per-peer rate budgets. Optional durable revocation stores only token digests in a separate SQLite database. Imported preview credentials remain supported, but no route accepts a principal header as authentication.

The service uses four workers and at most 64 queued connections, bounded keep-alive and socket timeouts, 16 KiB bodies, bounded per-principal and socket-peer rate-limit state, and a cooperative two-second execution budget. Excess requests return `429` with `Retry-After`; a full connection queue closes excess connections. It ignores forwarded IP/identity headers. Behind one proxy, the socket-peer budget is shared by all users. A signal monitor stops the listener outside the signal handler and joins on shutdown. Redacted request correlation and aggregate metrics are emitted; blocking calls and representative load still need staging qualification. The service schedules bounded idempotency cleanup every 60 seconds. Local auth requires `ICHESS_ACCOUNT_DB_PATH` in addition to the game and revocation database paths.

The raw SQLite handle is exposed for storage integration and tests; callers must hold the database mutex for concurrent access and must not close the handle themselves. Normal consumers should use the application interface.

## Security Requirements

- Bind the eventual listener only to loopback or a private container network behind the reverse proxy. Use Cloudflare Tunnel for ingress; do not make the backend port public.
- Keep authentication and authorization in the application boundary. Never derive identity from an unverified header, and never treat a game ID as a credential.
- The HTTP adapter requires CSRF-token shape, idempotency keys, strong version ETags, bounded JSON bodies, and strict DTO fields. JSON parsing rejects duplicate object keys (including escaped equivalents), nesting deeper than parser depth 16, and numeric overflow with a redacted `400` response. `IRequestAuthenticator` must validate CSRF contents and `Origin`, not just their presence.
- `IGameApplication` must reauthorize every operation, atomically enforce expected versions and idempotency, and return safe `ApplicationError` details. Do not share unsynchronized mutable `ChessGameHandler` instances between requests.
- Responses are `no-store`; errors do not include exception messages. Do not log request bodies, cookies, CSRF tokens, or authorization material.
- `cpp-httplib` is used as a private HTTP origin behind a TLS-terminating reverse proxy. Do not enable direct public HTTP access.

## Configuration and Verification

`HttpApiConfig` clamps request-body limits to 1-65536 bytes and read/write timeouts to 1-60 seconds. History page limits are clamped to 1-200 items; omitted `limit` defaults to the smaller of 100 and the effective configured limit. These limits do not replace rate limiting or transaction/execution deadlines.

Build and run the backend tests from the repository root:

```sh
cmake --preset dev
cmake --build --preset dev-build --target test_ichess_runner
cmake --build --preset dev-build --target ichess_service
ctest --preset dev-test -R HttpApi --output-on-failure
```

Tests cover HTTP validation, malformed schemas, game and revocation readiness, concurrent versioned writes, bounded retention cleanup, retry authorization/expiry/rollback, restart repetition, promotion/checkmate, sessions, protected credential files, queue limits, and rate-limit concurrency. A real HTTP workflow combines session auth with SQLite gameplay and revocation. Run all tests with `ctest --preset dev-test --output-on-failure`. Linux deployment policy, OIDC verification, external ingress isolation, backup restore, and load qualification still require deployment testing. See [ROADMAP.md](ROADMAP.md).

## Dependencies

CMake FetchContent pins `cpp-httplib`, `nlohmann/json`, and the SQLite 3.50.4 amalgamation with archive hashes. SQLite is compiled as C with loadable extensions disabled. Review dependency updates and run the complete CTest suite when changing them. Dependency pinning does not replace vulnerability monitoring.