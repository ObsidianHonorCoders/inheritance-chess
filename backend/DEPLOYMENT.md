# Private Preview Deployment

## Supported Boundary

`ichess_service` is a native C++ origin for restricted, untimed preview play behind an HTTPS reverse proxy and Cloudflare Tunnel. It is not a Cloudflare Worker. Do not publish the origin port or use this credential-file mechanism as public account onboarding. Cloudflare Access may provide an additional preview gate; the application does not currently verify Access JWTs or use forwarded identity headers.

The Windows Debug build and local HTTP integration tests are verified. Linux filesystem policy, signals, process supervision, load, backups, and real Tunnel routing still require staging verification.

## Startup

Build `ichess_service` with the repository CMake preset. On Windows Debug its executable is `build/dev/backend/Debug/ichess_service.exe`. Supply these environment variables through the service manager:

| Variable | Requirement |
|---|---|
| `ICHESS_PUBLIC_ORIGIN` | Exact public HTTPS origin, for example `https://chess.example.com`, with no path or trailing slash. |
| `ICHESS_SESSION_FILE` | Absolute path to an owner-restricted JSON credential file. |
| `ICHESS_DB_PATH` | Absolute path to a SQLite file on local durable storage; parent directory must exist. |
| `ICHESS_PORT` | Optional integer 1-65535; defaults to 8080. Binding is always `127.0.0.1`. |

Missing or invalid configuration exits nonzero with a generic error. It never falls back to anonymous authentication or a public listener. Linux currently accepts ext-family, XFS, Btrfs, and ZFS backing filesystems; overlay/tmpfs/network storage is rejected by the runtime path policy. Verify your actual persistent mount before deployment.

## Preview Credentials

The session file is a nonempty JSON array. Every entry has exactly `principal`, `token`, and `expires`: a bounded ASCII principal identifier, a cryptographically generated 64-character hexadecimal bearer token, and an integer Unix expiry in seconds, after startup time and at most 24 hours away. Generate each token with a trusted OS cryptographic tool; do not use repeated characters or example credentials. The file is limited to 64 KiB and 1024 entries.

Keep this file out of Git, logs, images, frontend bundles, and shared folders. On Unix it must be a regular file owned by the service user with no group/other access (mode 0600). On Windows ownership must match the service account and its ACL may grant access only to that account, SYSTEM, and Administrators. Symlink/reparse credential files are rejected. Protect the parent directories as well; file checks do not protect against a privileged administrator.

Provision each imported token to its intended preview browser only through a trusted, same-origin HTTPS mechanism that sets `__Host-ichess_session` with `Secure; HttpOnly; Path=/; SameSite=Strict`, no `Domain`, and expiry no later than the server session. Do not create a public endpoint that simply accepts a principal or returns these credentials. No browser onboarding UI is supplied. The in-process trusted session API also returns a secure `Set-Cookie` value for future provider integration.

The browser obtains its session-bound CSRF token from `GET /api/v1/session`; writes require that token and the exact Origin. CSRF tokens regenerate when the service imports sessions after restart. Sessions are held in memory. Removing an entry from the file has no live effect: remove the credential and restart to revoke it in the preview executable. In-process integrations can call `revoke_session`; revocation is not persisted across a restart that reimports a still-valid credential. A production login/logout flow must handle rotation and durable revocation.

## Proxy and Tunnel

Use the topology `Cloudflare -> cloudflared -> private reverse proxy -> 127.0.0.1:8080`. Run the connector/proxy on the same host/network namespace as the loopback origin or use an explicit private transport; never change the bind to `0.0.0.0` to work around routing. Avoid other untrusted local applications on this host.

At the public reverse proxy:

- Block `/api/v1/health/` from external routing. Run health checks directly on loopback/private management access.
- Route `/api/` to the origin and serve frontend assets on the same HTTPS origin. Forward `Cookie`, `Origin`, CSRF, ETag, and idempotency headers unchanged. Reject malformed framing, limit bodies to 16 KiB, and bound connection/header/body timeouts.
- Disable caching for all `/api/` responses at both proxy and Cloudflare. Do not enable wildcard credentialed CORS.
- Strip client-supplied identity headers. The backend ignores forwarded IP headers and never derives identity from them.
- Apply tighter edge budgets to creation and expensive operations. Backend defaults are 120 requests/principal/minute and 240 requests/socket peer/minute. Behind a single proxy the peer allowance is shared across users, including probes. Tune and test through code/config review before expanding traffic; do not trust arbitrary forwarded addresses to split this budget.
- Add an explicit final unmatched Tunnel ingress rule returning 404 and firewall all direct-origin ingress. Use Access restrictions for the preview audience in addition to application sessions.

The backend has four workers and 64 queued connections. Queue overflow closes connections rather than promising a JSON response. Socket timeouts are not total application execution deadlines. History replay is bounded at 4096 plies but still needs representative load tests.

## Operations and Release Checks

Run as a dedicated least-privilege account. Protect the database directory, including SQLite WAL/SHM files and backups. Use a supervisor for automatic restart, secure environment injection, and resource limits. Stop via SIGINT/SIGTERM where supported; verify supervisor shutdown deadlines in staging.

Use SQLite's online backup API or an equivalent consistent snapshot procedure; do not copy only the main file while WAL writes are active. Test restore on a separate instance and verify participants, full move history/repetition, versions, outcomes, and retries. Encrypt backups and define retention. Request-driven cleanup removes bounded batches of expired retry records; add scheduled maintenance for inactive installations.

Before public release, complete provider-backed login/logout and rotation, durable revocation, token-protection review, per-operation budgets, execution deadlines, redacted request correlation/metrics, dependency vulnerability review, Linux CI, load tests, backup/restore drills, and external verification that origin/health bypasses and edge caching are impossible. Timed games remain rejected with `422`; do not advertise them.