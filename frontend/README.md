# Inheritance Chess Frontend

The React/Vite frontend calls the C++ backend through Vite's same-origin `/api` proxy. Chess
positions and legal moves come from the backend/domain; the frontend does not implement chess
rules.

## Run locally on Windows

From the repository root, build and start the backend in one PowerShell terminal:

```powershell
cmake -S . -B build -DICHESS_BUILD_BACKEND=ON
cmake --build build --target ichess_backend_server
.\build\backend\ichess_backend_server.exe
```

The backend binds only to `127.0.0.1:8080`. Keep it running. In a second terminal:

```powershell
cd frontend
npm ci
npm run dev
```

Open the Vite URL shown in the terminal (normally http://localhost:5173). To run type-check and a
production build, use `npm run typecheck` and `npm run build` in `frontend/`.

The service provides **local-development-only** guest sessions and in-memory games. They are lost
when the backend exits. To play both sides, open the frontend in a different browser or private
window, create a separate guest session, then join the first player's game ID. The first player
must share that ID.

Do not bind this service to a LAN/public interface, place it behind Cloudflare Tunnel, or use it
for real accounts. It has no production identity provider or durable game/session storage. See
[the backend README](../backend/README.md) and [the migration gaps report](../docs/FRONTEND_MIGRATION_GAPS.md).
