# Backend Adapter

`ichess_backend` is the C++ HTTP/JSON adapter for the contract in [openapi.yaml](openapi.yaml). It links the existing game-handler and translation-unit targets, but keeps transport DTOs separate from chess-core types.

## Build Boundary

- `include/http_api.h` registers versioned API routes on a `cpp-httplib` server.
- `IRequestAuthenticator` must validate the server-side session and, on writes, CSRF token and allowed origin. The interface intentionally has no permissive default implementation.
- `IGameApplication` owns authorization, versioned/idempotent operations, and repository transactions. Its implementation must enforce game-seat ownership and persist initial FEN plus the ordered move log.
- `api_types.h` contains transport-neutral API DTOs and safe application errors.

The host application supplies concrete authentication and game-application implementations before starting a listener. This library does not include a development identity bypass or an in-memory production store. A partial SQLite implementation now supports durable untimed creation and participant-only snapshots, but it is not a complete game service. Until authentication and the remaining application operations exist, the service must not be exposed through Cloudflare Tunnel.

## SQLite Application Foundation

`SqliteDatabase` owns a full-mutex SQLite connection with foreign keys, defensive mode, untrusted schema, WAL, full synchronization, and a five-second busy timeout. Schema version 1 includes games, ordered moves, and idempotency records. Unsupported versions fail initialization and roll back schema changes. The database must outlive every `SqliteGameApplication` referencing it. Keep the database on local durable storage, not a network filesystem.

`SqliteGameApplication` currently implements `create_game` and `get_game`:

- Untimed creation is atomic with its idempotency record. A retry scoped to the same principal and operation returns the stored original snapshot; a different initial FEN under the same key returns `409`.
- Idempotency results remain eligible for replay for 24 hours. An expired key may create a new game. Successful create transactions clean up at most 64 expired records, plus the expired record matching the current key. This bounds per-request cleanup work, not total storage; scheduled maintenance is still required for inactive deployments.
- Retries recheck current game visibility before replay. Unknown and non-visible games both return `404`. Storage failures return redacted errors rather than being presented as missing games.
- Public IDs use 128 bits from the OS random source: BCrypt on Windows and `/dev/urandom` on Unix-like systems. IDs are not credentials.
- Handlers are reconstructed per read from initial FEN and contiguous ordered moves. A read transaction keeps the game metadata and move log in one consistent database snapshot. Wrappers sharing a database synchronize on its connection mutex; independent connections serialize creation through SQLite write transactions.
- Initial FEN must round-trip through the core parser in canonical form and pass basic chess-position checks. These checks are not a proof of historical reachability. Initial terminal positions do not advertise playable moves.

Timed creation returns `422` until restart-safe clock behavior is implemented. Join, legal-move/history endpoints, move submission, resignation, and draw actions currently return `501` from this partial application. The readiness check reflects successful database initialization, not a continuous storage health probe. The host must not treat it as production readiness yet.

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
ctest --preset dev-test -R HttpApi --output-on-failure
```

The HTTP tests use test authentication and application implementations. Real SQLite tests additionally cover initialization/reopen, durable create/read, concurrent duplicate creation, replay authorization, idempotency expiry, rollback, and basic initial-position validation. Run those with `ctest --preset dev-test -R Sqlite --output-on-failure`. Production identity verification, game-write races, and deployment isolation are not covered yet. See [ROADMAP.md](ROADMAP.md) for the remaining implementation and release gates.

## Dependencies

CMake FetchContent pins `cpp-httplib`, `nlohmann/json`, and the SQLite 3.50.4 amalgamation with archive hashes. SQLite is compiled as C with loadable extensions disabled. Review dependency updates and run the complete CTest suite when changing them. Dependency pinning does not replace vulnerability monitoring.