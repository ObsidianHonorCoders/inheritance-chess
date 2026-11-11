# Backend Adapter

`ichess_backend` is the C++ HTTP/JSON adapter for the contract in [openapi.yaml](openapi.yaml). It links the existing game-handler and translation-unit targets, but keeps transport DTOs separate from chess-core types.

## Build Boundary

- `include/http_api.h` registers versioned API routes on a `cpp-httplib` server.
- `IRequestAuthenticator` must validate the server-side session and, on writes, CSRF token and allowed origin. The interface intentionally has no permissive default implementation.
- `IGameApplication` owns authorization, versioned/idempotent operations, and repository transactions. Its implementation must enforce game-seat ownership and persist initial FEN plus the ordered move log.
- `api_types.h` contains transport-neutral API DTOs and safe application errors.

The `ichess_backend_server` executable provides **local-development-only** implementations in
`src/local_adapters.cpp`: ephemeral guest-cookie sessions and an in-memory game application that
delegates move validation and game state to the C++ domain. It binds to `127.0.0.1` only. Games
and sessions are lost at process exit. This is not a production host and must not be exposed to a
LAN, reverse proxy, or Cloudflare Tunnel.

Production hosting still requires a trusted identity/session implementation, durable game
repository, bounded session/idempotency retention, and deployment-specific origin/cookie settings.

## Run the local development service

From the repository root:

```powershell
cmake -S . -B build -DICHESS_BUILD_BACKEND=ON
cmake --build build --target ichess_backend_server
.\build\backend\ichess_backend_server.exe
```

The default port is 8080. Set `ICHESS_BACKEND_PORT` to choose another local port; update the
frontend Vite proxy target in `frontend/vite.config.ts` to match.

The local-only `POST /api/v1/dev/session` endpoint issues a guest cookie for the current browser.
It accepts requests only from the configured localhost Vite origins. The endpoint deliberately
does not exist in the production API contract.

## Security Requirements

- Bind the eventual listener only to loopback or a private container network behind the reverse proxy. Use Cloudflare Tunnel for ingress; do not make the backend port public.
- Keep authentication and authorization in the application boundary. Never derive identity from an unverified header, and never treat a game ID as a credential.
- The HTTP adapter requires CSRF-token shape, idempotency keys, strong version ETags, bounded JSON bodies, and strict DTO fields. `IRequestAuthenticator` must validate CSRF contents and `Origin`, not just their presence.
- `IGameApplication` must reauthorize every operation, atomically enforce expected versions and idempotency, and return safe `ApplicationError` details. Do not share unsynchronized mutable `ChessGameHandler` instances between requests.
- Responses are `no-store`; errors do not include exception messages. Do not log request bodies, cookies, CSRF tokens, or authorization material.
- `cpp-httplib` is used as a private HTTP origin behind a TLS-terminating reverse proxy. Do not enable direct public HTTP access.

## Dependencies

CMake FetchContent pins `cpp-httplib` and `nlohmann/json` versions. Review dependency updates and run the complete CTest suite when changing them.