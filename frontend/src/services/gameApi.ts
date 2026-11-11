export interface ApiLegalMove {
  uci: string;
  san: string;
}

export interface GameSnapshot {
  gameId: string;
  version: number;
  fen: string;
  sideToMove: 'white' | 'black';
  status: string;
  legalMoves: ApiLegalMove[];
  moveCount: number;
  viewerSeat: 'white' | 'black';
  players: {
    white: { displayName: string };
    black: { displayName: string } | null;
  };
  clock: null | {
    whiteRemainingMs: number;
    blackRemainingMs: number;
    runningColor: string | null;
    incrementMs: number;
  };
  serverTime: string;
}

export interface ApiMoveRecord {
  ply: number;
  uci: string;
  san: string;
}

export class GameApiError extends Error {
  constructor(
    message: string,
    readonly status: number,
    readonly code: string,
    readonly currentVersion?: number,
  ) {
    super(message);
    this.name = 'GameApiError';
  }
}

let csrfToken: string | null = null;

function requestHeaders(init: RequestInit, stateChanging: boolean): Headers {
  const headers = new Headers(init.headers);
  if (stateChanging) {
    headers.set('Content-Type', 'application/json');
    if (!csrfToken) throw new Error('The session is not ready. Sign in as a local guest first.');
    headers.set('X-CSRF-Token', csrfToken);
    headers.set('Idempotency-Key', crypto.randomUUID());
  }
  return headers;
}

async function request<T>(path: string, init: RequestInit = {}, stateChanging = false): Promise<T> {
  const response = await fetch(path, {
    ...init,
    credentials: 'same-origin',
    headers: requestHeaders(init, stateChanging),
  });
  const contentType = response.headers.get('Content-Type') ?? '';
  const body: unknown = contentType.includes('json') ? await response.json() : null;

  if (!response.ok) {
    const problem =
      typeof body === 'object' && body !== null
        ? (body as { detail?: unknown; code?: unknown; currentVersion?: unknown })
        : {};
    const message = typeof problem.detail === 'string' ? problem.detail : `Request failed (${response.status}).`;
    const code = typeof problem.code === 'string' ? problem.code : 'request_failed';
    const currentVersion =
      typeof problem.currentVersion === 'number' ? problem.currentVersion : undefined;
    throw new GameApiError(message, response.status, code, currentVersion);
  }

  return body as T;
}

function get<T>(path: string): Promise<T> {
  return request<T>(path);
}

function post<T>(path: string, body: unknown, version?: number): Promise<T> {
  const headers = requestHeaders({}, true);
  if (version !== undefined) headers.set('If-Match', `"${version}"`);
  return request<T>(
    path,
    { method: 'POST', headers, body: JSON.stringify(body) },
    false,
  );
}

export const gameApi = {
  async getSession(): Promise<void> {
    const session = await get<{ csrfToken: string }>('/api/v1/session');
    csrfToken = session.csrfToken;
  },

  async startGuestSession(displayName: string): Promise<void> {
    const session = await request<{ csrfToken: string }>(
      '/api/v1/dev/session',
      {
        method: 'POST',
        headers: new Headers({ 'Content-Type': 'application/json' }),
        body: JSON.stringify({ displayName }),
      },
    );
    csrfToken = session.csrfToken;
  },

  createGame(): Promise<GameSnapshot> {
    return post<GameSnapshot>('/api/v1/games', {});
  },

  getGame(gameId: string): Promise<GameSnapshot> {
    return get<GameSnapshot>(`/api/v1/games/${encodeURIComponent(gameId)}`);
  },

  joinGame(gameId: string, version: number): Promise<GameSnapshot> {
    return post<GameSnapshot>(`/api/v1/games/${encodeURIComponent(gameId)}`, {}, version);
  },

  async getMoveHistory(gameId: string): Promise<ApiMoveRecord[]> {
    const allMoves: ApiMoveRecord[] = [];
    let afterPly = 0;
    while (true) {
      const page = await get<{
        moves: ApiMoveRecord[];
        nextAfterPly: number | null;
      }>(`/api/v1/games/${encodeURIComponent(gameId)}/moves?afterPly=${afterPly}&limit=200`);
      allMoves.push(...page.moves);
      if (page.nextAfterPly === null) return allMoves;
      afterPly = page.nextAfterPly;
    }
  },

  submitMove(game: GameSnapshot, uci: string): Promise<GameSnapshot> {
    return post<GameSnapshot>(
      `/api/v1/games/${encodeURIComponent(game.gameId)}/moves`,
      { uci },
      game.version,
    );
  },

  resign(game: GameSnapshot): Promise<GameSnapshot> {
    return post<GameSnapshot>(
      `/api/v1/games/${encodeURIComponent(game.gameId)}/resign`,
      {},
      game.version,
    );
  },
};
