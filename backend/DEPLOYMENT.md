# Private Preview Deployment

## Supported Boundary

`ichess_service` is a native C++ origin for restricted, untimed play behind an HTTPS reverse proxy and Cloudflare Tunnel. It is not a Cloudflare Worker. Do not publish the origin port. Local username/password registration and login are supported; provider identities and account recovery are not. Cloudflare Access may provide an additional gate, but the application does not currently verify Access JWTs or use forwarded identity headers.

The repository CI builds and tests Linux, Windows, and macOS; the local Windows Debug build and HTTP integration tests are verified. Linux filesystem policy, signals, process supervision, load, backups, and real Tunnel routing still require staging verification.

## Startup

Build `ichess_service` with the repository CMake preset. On Windows Debug its executable is `build/dev/backend/Debug/ichess_service.exe`. Supply these environment variables through the service manager:

| Variable | Requirement |
|---|---|
| `ICHESS_PUBLIC_ORIGIN` | Exact public HTTPS origin, for example `https://chess.example.com`, with no path or trailing slash. |
| `ICHESS_SESSION_FILE` | Optional absolute path to an owner-restricted JSON credential file for operator-provisioned preview sessions. Omit for local accounts only. |
| `ICHESS_DB_PATH` | Absolute path to a SQLite file on local durable storage; parent directory must exist. |
| `ICHESS_REVOCATION_DB_PATH` | Required absolute path to a dedicated revocation SQLite file in the same protected directory as the game database. |
| `ICHESS_ACCOUNT_DB_PATH` | Required absolute path to a dedicated SQLite account database in that same protected directory. |
| `ICHESS_ALLOW_REGISTRATION` | Optional `true` or `false`; defaults to `true`. Set `false` to close account creation while retaining login. |
| `ICHESS_PORT` | Optional integer 1-65535; defaults to 8080. Binding is always `127.0.0.1`. |

Missing or invalid configuration exits nonzero with a generic error. It never falls back to anonymous authentication or a public listener. Linux currently accepts ext-family, XFS, Btrfs, and ZFS backing filesystems; overlay/tmpfs/network storage is rejected by the runtime path policy. Verify your actual persistent mount before deployment.

`deployment/ichess.service` is a systemd starting point with a dedicated account, restricted writable state directory, restart policy, and service hardening. Review it against the target distribution, install the executable at `/opt/inheritance-chess/bin/ichess_service`, and provision `/etc/ichess/service.env` with mode `0640` readable by the service group. Point the session file and all three databases into `/var/lib/ichess`; create any imported session file as the service user with mode `0600`. The unit is a template, not a verified host configuration.

## Preview Credentials

The optional preview session file is a nonempty JSON array. Every entry has exactly `principal`, `token`, and `expires`: a bounded ASCII principal identifier, a cryptographically generated 64-character hexadecimal bearer token, and an integer Unix expiry in seconds, after startup time and at most 24 hours away. Generate each token with a trusted OS cryptographic tool; do not use repeated characters or example credentials. The file is limited to 64 KiB and 1024 entries. Local users can register at `POST /api/v1/auth/register` or log in at `POST /api/v1/auth/login`; both require the exact configured `Origin`. Usernames are 3-32 ASCII letters/digits/period/underscore/hyphen, case-insensitive, and passwords must be 12-1024 bytes. Passwords are stored as salted PBKDF2-HMAC-SHA256 hashes, never plaintext; hash format version 1 uses 600,000 iterations, and the account DB migrates existing v1 hashes into that version marker. No plaintext password is persisted. Both endpoints set the secure session cookie and return a CSRF token. `POST /api/v1/auth/password` requires the current session, CSRF token, and current password; success changes the password, revokes all current sessions for that account, and issues a new cookie. There is no forgot-password/reset route because no verified recovery channel is configured. Login, registration and password change are protected by per-peer and global hash-work limits. `POST /api/v1/auth/logout` requires that session's CSRF token and revokes it.

Keep this file out of Git, logs, images, frontend bundles, and shared folders. On Unix it must be a regular file owned by the service user with no group/other access (mode 0600). On Windows ownership must match the service account and its ACL may grant access only to that account, SYSTEM, and Administrators. Symlink/reparse credential files are rejected. Protect the parent directories as well; file checks do not protect against a privileged administrator.

Provision each imported token to its intended preview browser only through a trusted, same-origin HTTPS mechanism that sets `__Host-ichess_session` with `Secure; HttpOnly; Path=/; SameSite=Strict`, no `Domain`, and expiry no later than the server session. Do not create a public endpoint that simply accepts a principal or returns these credentials. No browser onboarding UI is supplied. The in-process trusted session API also returns a secure `Set-Cookie` value for future provider integration.

The browser obtains its session-bound CSRF token from `GET /api/v1/session`; writes require that token and the exact Origin. Password changes use `POST /api/v1/auth/password` with `currentPassword` and `newPassword`, and revoke the account's existing sessions before issuing a replacement. There is no out-of-band reset route without a verified delivery channel. CSRF tokens regenerate when the service imports sessions after restart. Session credentials are held in memory, while revoked token digests are persisted in the dedicated revocation database. Removing an entry from the file has no live effect: remove the credential and restart to stop importing it. In-process integrations can call `revoke_session`; persisted revocation prevents a still-valid imported credential from becoming active again after restart.

## Proxy and Tunnel

Use the topology `Cloudflare -> cloudflared -> private reverse proxy -> 127.0.0.1:8080`. Run the connector/proxy on the same host/network namespace as the loopback origin or use an explicit private transport; never change the bind to `0.0.0.0` to work around routing. Avoid other untrusted local applications on this host.

At the public reverse proxy:

- Block `/api/v1/health/` from external routing. Run health checks directly on loopback/private management access.
- Route `/api/` to the origin and serve frontend assets on the same HTTPS origin. Forward `Cookie`, `Origin`, CSRF, ETag, and idempotency headers unchanged. Reject malformed framing, limit bodies to 16 KiB, and bound connection/header/body timeouts.
- Disable caching for all `/api/` responses at both proxy and Cloudflare. Do not enable wildcard credentialed CORS.
- Strip client-supplied identity headers. The backend ignores forwarded IP headers and never derives identity from them.
- Apply tighter edge budgets to creation and expensive operations. Backend defaults are 120 requests/principal/minute and 240 requests/socket peer/minute. Behind a single proxy the peer allowance is shared across users, including probes. Tune and test through code/config review before expanding traffic; do not trust arbitrary forwarded addresses to split this budget.
- Add an explicit final unmatched Tunnel ingress rule returning 404 and firewall all direct-origin ingress. Use Access restrictions for the preview audience in addition to application sessions.

The backend has four workers and 64 queued connections. Queue overflow closes connections rather than promising a JSON response. Readiness covers game, revocation, and account storage, including SQLite journal/sync/security settings. The two-second execution budget is cooperative at application checkpoints; it does not interrupt blocking OS calls, and SQLite lock waits are capped at one second. History replay is bounded at 4096 plies but still needs representative load tests.

## Operations and Release Checks

Run as a dedicated least-privilege account. Protect the database directory, including SQLite WAL/SHM files and backups. Use a supervisor for automatic restart, secure environment injection, and resource limits. Stop via SIGINT/SIGTERM where supported; verify supervisor shutdown deadlines in staging.

Use each store's `backup_to(path)` online backup API or an equivalent consistent snapshot procedure for all three databases; do not copy only the main file while WAL writes are active. The helper requires a new destination and never overwrites an existing backup. Stop or quiesce writes while taking the three files so they represent a coherent recovery point. Test restore on a separate instance and verify accounts, participants, full move history/repetition, versions, outcomes, retries, and revocation behavior. A stale revocation backup can restore a previously revoked but unexpired credential, so rotate/remove affected credentials after recovery from an older snapshot. Encrypt backups and define retention. The service schedules bounded idempotency cleanup every 60 seconds; monitor its generic failure event and retain an offline cleanup procedure for extended outages.

Before public release, review the account registration/recovery policy, password-hash cost under expected load, credential protection, and dependency vulnerability posture; complete Linux deployment verification, representative load tests, backup/restore drills for all three databases, and external verification that origin/health bypasses and edge caching are impossible. CI builds/tests on Linux, Windows, and macOS. Request budgets, cooperative execution checkpoints, redacted request correlation, and aggregate metrics are implemented but still require production load qualification. Social login is optional and can later authenticate through the session-issuer interface. Timed games remain rejected with `422`; do not advertise them.