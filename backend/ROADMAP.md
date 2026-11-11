# Backend Implementation Roadmap

## Current State

The repository has seven C++20 chess-core libraries, two console executables, and an HTTP adapter library. The adapter exposes the versioned API defined in [openapi.yaml](openapi.yaml), with transport DTOs and interfaces for authentication and game operations. HTTP tests use doubles for those interfaces. A partial `SqliteGameApplication` supports durable untimed creation and participant snapshots, with real-database tests for duplicate-create concurrency, restart, retry authorization, expiry, and rollback. There is no production service executable, concrete authenticator, or complete durable game application yet.

The core `ChessGameHandler` owns mutable board, timer, draw-offer, and repetition state. Do not share it concurrently between requests. Persist the initial FEN and ordered move log; reconstructing from the latest FEN alone loses repetition history. Keep public game identifiers separate from the core's identifiers.

The behavioral and security requirements remain in [SERVICE_CONTRACT.md](../docs/SERVICE_CONTRACT.md). This roadmap orders implementation work; it does not change the API contract.

## 1. Durable Game Application

Implemented foundation: pinned SQLite, schema version validation, synchronized connection ownership, atomic idempotent untimed creation, participant-only reads, 128-bit OS-generated IDs, and disposable replay-based handlers. Creation retries are retained for 24 hours. Next implement join and move transactions, the remaining read and lifecycle operations, full move-log restoration tests, clock policy, scheduled retention cleanup, and operational readiness checks. Other application operations currently fail closed with `501`; timed creation returns `422`.

- Implement `IGameApplication` with participant authorization on every operation, including idempotent replay. Return the same `404` for absent and non-visible games.
- Start with SQLite on local durable storage and one API process unless multiple instances are already required. Use PostgreSQL for multiple writers or API instances. Use parameterized queries, migrations, foreign keys, uniqueness constraints, and bounded lock waits.
- Store participants, initial FEN, ordered UCI/SAN history, version, outcome, draw offers, clock state, and timestamps. Generate public game IDs with a cryptographic RNG and at least 128 bits of entropy.
- Atomically check the version, validate the action, append history, update game state, and store the idempotency result. Scope keys by principal and operation/resource; reject different commands under the same key. Document retention and cleanup.
- Use isolated reconstructed handlers initially. Introduce a bounded disposable cache only after measurements show replay is too expensive; storage remains authoritative.
- Settle clock policy before implementing timed games. Persist authoritative timing data so restart and reconnect cannot reset a clock.

Acceptance: competing writes to one version yield exactly one mutation; retries do not duplicate games or moves; failed transactions leave state unchanged; games restore after restart, including repetition and terminal state.

## 2. Verified Browser Sessions

- Choose an established OIDC provider for public accounts or a verified Cloudflare Access integration for a private preview. Do not implement an identity-header bypass.
- Implement `IRequestAuthenticator` against expiring, revocable server-side sessions. Use opaque random tokens and store only protected token representations. Set `__Host-ichess_session` with `Secure`, `HttpOnly`, `Path=/`, no `Domain`, and a deliberate `SameSite` policy.
- Verify session-bound CSRF tokens and the exact configured public `Origin` on writes. Rotate sessions at login and privilege changes, and invalidate them on logout.
- Validate any Access assertions with a maintained verification library, including signature, issuer, audience, and expiry. Never trust `CF-Connecting-IP` or forwarded identity headers outside the trusted connector/proxy path.

Acceptance: expired, revoked, forged, and missing sessions fail closed; cross-origin writes and invalid CSRF tokens are rejected; users cannot act for an opponent or read unrelated games.

## 3. Bounded Service Runtime

- Add a service executable that supplies the concrete adapters, validates startup configuration, and binds only to loopback or a private container interface.
- Bound worker count, queued requests, active connections, keep-alive behavior, database waits, and execution time. Return controlled overload errors rather than growing memory without limit.
- Add backend rate budgets by principal, with trusted source IP as a secondary signal. Apply tighter limits to creation and expensive actions. Bound limiter state and return `429` with `Retry-After` when available.
- Support graceful shutdown, readiness tied to storage, request correlation, latency/error metrics, and redacted structured logging. Do not log cookies, tokens, or request bodies.

Acceptance: load tests demonstrate bounded memory and predictable overload behavior; dependency failure affects readiness; shutdown does not lose committed data or expose internal diagnostics.

## 4. Private Cloudflare Deployment

The intended model is a native C++ origin service on a host/container behind Cloudflare Tunnel, not a native executable hosted in Cloudflare Workers.

- Serve static assets and `/api/*` under one public HTTPS origin through a private reverse proxy. Keep liveness/readiness routes private and add an explicit unmatched Tunnel ingress rule.
- Close public origin ports. Restrict origin access to the intended connector/proxy network; verify from outside the host that no bypass path exists.
- Disable edge caching for authenticated API traffic, retain `private, no-store`, and avoid wildcard credentialed CORS. Apply edge WAF/rate controls in addition to backend controls.
- Run as a non-root user, inject secrets outside source/images, pin and scan dependencies/images, and maintain tested backups and a restore procedure.

Acceptance: direct-origin access fails; cookies and origin checks work through the real proxy; game/session responses are not cached; restore and rollback procedures succeed in staging.

## Release Gate

Do not expose the current adapter as a production service until phases 1-4 are implemented and verified. Keep engine search, spectators, SSE, distributed caches, and message brokers out of the first release unless a demonstrated requirement justifies them.