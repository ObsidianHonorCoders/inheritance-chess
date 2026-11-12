# Backend Implementation Roadmap

## Current State

The repository has seven C++20 chess-core libraries, two console executables, an HTTP adapter library, and a loopback-only `ichess_service` private-preview executable. `SqliteGameApplication` implements all untimed game operations with transactional versions and retries. Concrete server-side sessions, bounded queues/rate budgets, strict HTTP validation, schema checks, and live readiness are implemented and covered by tests. Public account onboarding and timed games are not implemented. See [DEPLOYMENT.md](DEPLOYMENT.md) for the private-preview boundary.

The core `ChessGameHandler` owns mutable board, timer, draw-offer, and repetition state. Do not share it concurrently between requests. Persist the initial FEN and ordered move log; reconstructing from the latest FEN alone loses repetition history. Keep public game identifiers separate from the core's identifiers.

The behavioral and security requirements remain in [SERVICE_CONTRACT.md](../docs/SERVICE_CONTRACT.md). This roadmap orders implementation work; it does not change the API contract.

## 1. Durable Game Application

Implemented: pinned SQLite, full schema validation, synchronized connection ownership, all atomic idempotent untimed operations, participant authorization, 128-bit OS IDs, bounded history replay, restart repetition, lifecycle outcomes, scheduled bounded retry-record cleanup, and live readiness. Retry eligibility lasts 24 hours. Remaining: clock policy and persistence, disk/write health monitoring, backup/restore drills, and performance qualification. Timed creation returns `422`.

- Implement `IGameApplication` with participant authorization on every operation, including idempotent replay. Return the same `404` for absent and non-visible games.
- Start with SQLite on local durable storage and one API process unless multiple instances are already required. Use PostgreSQL for multiple writers or API instances. Use parameterized queries, migrations, foreign keys, uniqueness constraints, and bounded lock waits.
- Store participants, initial FEN, ordered UCI/SAN history, version, outcome, draw offers, clock state, and timestamps. Generate public game IDs with a cryptographic RNG and at least 128 bits of entropy.
- Atomically check the version, validate the action, append history, update game state, and store the idempotency result. Scope keys by principal and operation/resource; reject different commands under the same key. Document retention and cleanup.
- Use isolated reconstructed handlers initially. Introduce a bounded disposable cache only after measurements show replay is too expensive; storage remains authoritative.
- Settle clock policy before implementing timed games. Persist authoritative timing data so restart and reconnect cannot reset a clock.

Acceptance: competing writes to one version yield exactly one mutation; retries do not duplicate games or moves; failed transactions leave state unchanged; games restore after restart, including repetition and terminal state.

## 2. Verified Browser Sessions

Implemented for private preview: bounded opaque server-side sessions, expiry, optional durable revocation across restart, strict cookies, exact Origin and CSRF verification, and owner-restricted credential-file import. No public login route or trusted identity-header shortcut exists. Remaining: provider-backed onboarding, login/logout routes, rotation, and protected token representations for persistent account sessions. File-provisioned identities are operator assignments, not OIDC assertions.

- Choose an established OIDC provider for public accounts or a verified Cloudflare Access integration for a private preview. Do not implement an identity-header bypass.
- Implement `IRequestAuthenticator` against expiring, revocable server-side sessions. Use opaque random tokens and store only protected token representations. Set `__Host-ichess_session` with `Secure`, `HttpOnly`, `Path=/`, no `Domain`, and a deliberate `SameSite` policy.
- Verify session-bound CSRF tokens and the exact configured public `Origin` on writes. Rotate sessions at login and privilege changes, and invalidate them on logout.
- Validate any Access assertions with a maintained verification library, including signature, issuer, audience, and expiry. Never trust `CF-Connecting-IP` or forwarded identity headers outside the trusted connector/proxy path.

Acceptance: expired, revoked, forged, and missing sessions fail closed; cross-origin writes and invalid CSRF tokens are rejected; users cannot act for an opponent or read unrelated games.

## 3. Bounded Service Runtime

Implemented: loopback-only service, strict startup configuration, protected preview credentials, four workers/64 queued connections, keep-alive/socket/body limits, bounded global and per-operation rate state, cooperative execution budgets, redacted request correlation/aggregate metrics, storage readiness for both databases, scheduled bounded retry cleanup, and signal-monitor shutdown. A systemd hardening template is available. Remaining: interruption of blocking operations, graceful-drain/load qualification, Linux deployment verification, and overload capacity measurements. CI builds/tests on Linux, Windows, and macOS. Socket-peer limits intentionally aggregate traffic through one proxy; forwarded IPs are not trusted.

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

Do not expose the private-preview service as a public production application until the remaining identity, operational, and deployment gates are verified. Timed play is outside the current supported subset. Keep engine search, spectators, SSE, distributed caches, and message brokers out of the first release unless a demonstrated requirement justifies them.