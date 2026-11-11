import { useCallback, useEffect, useMemo, useState, type FormEvent } from 'react';
import { useSearchParams } from 'react-router-dom';
import { Copy, RotateCcw, Swords } from 'lucide-react';
import { ChessBoard } from '@/components/chessboard/ChessBoard';
import { PageTransition } from '@/components/layout/PageTransition';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { useCopyToClipboard } from '@/hooks/useCopyToClipboard';
import { GameApiError, gameApi, type ApiMoveRecord, type GameSnapshot } from '@/services/gameApi';
import { getSoundEvent, soundService } from '@/services/soundService';
import { useSettingsStore } from '@/store/settingsStore';
import type { LegalMove, PromotionPiece, Square } from '@/types';
import { getTurnFromFen, START_FEN } from '@/utils/fen';

function displayError(error: unknown): string {
  return error instanceof Error ? error.message : 'The request could not be completed.';
}

function toSquare(value: string): Square | null {
  return /^[a-h][1-8]$/.test(value) ? (value as Square) : null;
}

function promotionPiece(value: string): PromotionPiece | undefined {
  return value === 'q' || value === 'r' || value === 'b' || value === 'n' ? value : undefined;
}

function legalMovesForBoard(game: GameSnapshot | null): LegalMove[] {
  if (!game) return [];
  return game.legalMoves.flatMap(({ uci, san }) => {
    const from = toSquare(uci.slice(0, 2));
    const to = toSquare(uci.slice(2, 4));
    if (!from || !to) return [];
    return [{
      from,
      to,
      san,
      promotion: promotionPiece(uci.slice(4, 5)),
      isCapture: san.includes('x'),
    }];
  });
}

function kingSquare(fen: string, color: 'w' | 'b'): Square | null {
  const rows = fen.split(' ')[0]?.split('/') ?? [];
  const king = color === 'w' ? 'K' : 'k';
  for (let rowIndex = 0; rowIndex < rows.length; rowIndex++) {
    let file = 0;
    for (const piece of rows[rowIndex] ?? '') {
      if (piece >= '1' && piece <= '8') {
        file += Number(piece);
        continue;
      }
      if (piece === king) {
        const square = toSquare(`${String.fromCharCode(97 + file)}${8 - rowIndex}`);
        return square;
      }
      file += 1;
    }
  }
  return null;
}

function turnLabel(game: GameSnapshot): string {
  if (game.status === 'checkmate') return 'Checkmate';
  if (game.status === 'stalemate') return 'Stalemate';
  if (game.status === 'draw' || game.status.startsWith('draw_')) return 'Draw';
  if (game.status.endsWith('_resigned')) return 'Game resigned';
  if (game.status.endsWith('_timeout')) return 'Time expired';
  return game.status === 'check' ? `${game.sideToMove} to move — check` : `${game.sideToMove} to move`;
}

function isGameOver(status: string): boolean {
  return status !== 'ongoing' && status !== 'check';
}

export function PlayPage() {
  const [searchParams, setSearchParams] = useSearchParams();
  const gameIdFromUrl = searchParams.get('gameId') ?? '';
  const orientation = useSettingsStore((state) => state.boardOrientation);
  const coordinatesVisible = useSettingsStore((state) => state.coordinatesVisible);
  const flipBoard = useSettingsStore((state) => state.flipBoard);
  const { copied, copy } = useCopyToClipboard();
  const [sessionReady, setSessionReady] = useState(false);
  const [sessionChecked, setSessionChecked] = useState(false);
  const [displayName, setDisplayName] = useState('');
  const [joinCode, setJoinCode] = useState(gameIdFromUrl);
  const [game, setGame] = useState<GameSnapshot | null>(null);
  const [history, setHistory] = useState<ApiMoveRecord[]>([]);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const boardMoves = useMemo(() => legalMovesForBoard(game), [game]);
  const lastMove = useMemo(() => {
    const uci = history.at(-1)?.uci;
    if (!uci) return null;
    const from = toSquare(uci.slice(0, 2));
    const to = toSquare(uci.slice(2, 4));
    return from && to ? { from, to } : null;
  }, [history]);

  useEffect(() => {
    let active = true;
    gameApi.getSession()
      .then(() => {
        if (active) setSessionReady(true);
      })
      .catch((requestError: unknown) => {
        if (!active) return;
        if (!(requestError instanceof GameApiError && requestError.status === 401)) {
          setError(displayError(requestError));
        }
        setSessionReady(false);
      })
      .finally(() => {
        if (active) setSessionChecked(true);
      });
    return () => {
      active = false;
    };
  }, []);

  const refreshGame = useCallback(async (gameId: string) => {
    const [snapshot, moves] = await Promise.all([
      gameApi.getGame(gameId),
      gameApi.getMoveHistory(gameId),
    ]);
    setGame((current) => current && current.version > snapshot.version ? current : snapshot);
    setHistory((current) => current.length > moves.length ? current : moves);
    setError(null);
  }, []);

  useEffect(() => {
    if (!sessionReady || !gameIdFromUrl) return;
    let active = true;
    const refresh = async () => {
      try {
        const [snapshot, moves] = await Promise.all([
          gameApi.getGame(gameIdFromUrl),
          gameApi.getMoveHistory(gameIdFromUrl),
        ]);
        if (active) {
          setGame((current) => current && current.version > snapshot.version ? current : snapshot);
          setHistory((current) => current.length > moves.length ? current : moves);
          setError(null);
        }
      } catch (requestError) {
        if (active) setError(displayError(requestError));
      }
    };
    void refresh();
    const timer = window.setInterval(() => void refresh(), 2500);
    return () => {
      active = false;
      window.clearInterval(timer);
    };
  }, [gameIdFromUrl, sessionReady]);

  const handleGuestSession = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    setBusy(true);
    setError(null);
    try {
      await gameApi.startGuestSession(displayName.trim());
      setSessionReady(true);
    } catch (requestError) {
      setError(displayError(requestError));
    } finally {
      setBusy(false);
    }
  };

  const handleCreateGame = async () => {
    setBusy(true);
    setError(null);
    try {
      const created = await gameApi.createGame();
      setGame(created);
      setHistory([]);
      setJoinCode(created.gameId);
      setSearchParams({ gameId: created.gameId });
    } catch (requestError) {
      setError(displayError(requestError));
    } finally {
      setBusy(false);
    }
  };

  const handleJoinGame = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    const gameId = joinCode.trim();
    if (!gameId) return;
    setBusy(true);
    setError(null);
    try {
      let joined: GameSnapshot;
      try {
        joined = await gameApi.joinGame(gameId, 1);
      } catch (requestError) {
        if (!(requestError instanceof GameApiError) || requestError.currentVersion === undefined) {
          throw requestError;
        }
        joined = await gameApi.joinGame(gameId, requestError.currentVersion);
      }
      const moves = await gameApi.getMoveHistory(gameId);
      setGame(joined);
      setHistory(moves);
      setSearchParams({ gameId });
    } catch (requestError) {
      setError(displayError(requestError));
    } finally {
      setBusy(false);
    }
  };

  const handleMove = useCallback((move: { from: Square; to: Square; promotion?: PromotionPiece }) => {
    if (!game || busy || game.viewerSeat !== game.sideToMove || !game.players.black) return false;
    setBusy(true);
    setError(null);
    const uci = `${move.from}${move.to}${move.promotion ?? ''}`;
    void gameApi.submitMove(game, uci)
      .then(async (updated) => {
        setGame(updated);
        const moveRecord = game.legalMoves.find((candidate) => candidate.uci === uci);
        if (moveRecord) soundService.play(getSoundEvent(moveRecord, updated.status));
        setHistory(await gameApi.getMoveHistory(updated.gameId));
      })
      .catch((requestError: unknown) => {
        setError(displayError(requestError));
        void refreshGame(game.gameId).catch((refreshError: unknown) => setError(displayError(refreshError)));
      })
      .finally(() => setBusy(false));
    return true;
  }, [busy, game, refreshGame]);

  const handleResign = async () => {
    if (!game) return;
    setBusy(true);
    setError(null);
    try {
      setGame(await gameApi.resign(game));
    } catch (requestError) {
      setError(displayError(requestError));
    } finally {
      setBusy(false);
    }
  };

  if (!sessionChecked) {
    return <PageTransition className="mx-auto max-w-xl py-12 text-center">Connecting to the local game service…</PageTransition>;
  }

  return (
    <PageTransition className="mx-auto max-w-6xl">
      <header className="mb-5 flex flex-wrap items-center justify-between gap-3">
        <div>
          <h1 className="text-2xl font-bold tracking-tight">Play</h1>
          <p className="text-sm text-muted-foreground">Moves and legal-move validation run in the C++ backend.</p>
        </div>
        {sessionReady && (
          <Button type="button" onClick={() => void handleCreateGame()} disabled={busy}>
            <Swords />
            New game
          </Button>
        )}
      </header>

      {error && <p className="mb-4 rounded-md border border-destructive/50 bg-destructive/10 p-3 text-sm" role="alert">{error}</p>}

      {!sessionReady ? (
        <form className="mx-auto max-w-md space-y-3 rounded-lg border bg-card p-5" onSubmit={(event) => void handleGuestSession(event)}>
          <h2 className="text-lg font-semibold">Local guest session</h2>
          <p className="text-sm text-muted-foreground">
            This development service keeps sessions and games in memory. It is only available on this computer.
          </p>
          <div className="space-y-1.5">
            <Label htmlFor="guest-name">Display name</Label>
            <Input id="guest-name" value={displayName} maxLength={32} required onChange={(event) => setDisplayName(event.target.value)} />
          </div>
          <Button type="submit" disabled={busy}>Continue</Button>
        </form>
      ) : (
        <>
          <div className="mb-5 grid gap-4 rounded-lg border bg-card p-4 md:grid-cols-2">
            <form className="flex items-end gap-2" onSubmit={(event) => void handleJoinGame(event)}>
              <div className="min-w-0 flex-1 space-y-1.5">
                <Label htmlFor="join-code">Join as Black with a game ID</Label>
                <Input id="join-code" value={joinCode} maxLength={64} onChange={(event) => setJoinCode(event.target.value)} placeholder="Paste the shared game ID" />
              </div>
              <Button type="submit" variant="secondary" disabled={busy || !joinCode.trim()}>Join</Button>
            </form>
            {game && (
              <div className="flex min-w-0 items-end gap-2">
                <div className="min-w-0 flex-1 space-y-1.5">
                  <Label htmlFor="game-id">Share this game ID</Label>
                  <Input id="game-id" readOnly value={game.gameId} className="font-mono" />
                </div>
                <Button type="button" variant="outline" aria-label="Copy game ID" onClick={() => void copy(game.gameId)}>
                  <Copy />
                  {copied ? 'Copied' : 'Copy'}
                </Button>
              </div>
            )}
          </div>

          {game ? (
            <div className="grid items-start gap-6 lg:grid-cols-[minmax(0,1fr)_20rem]">
              <section className="mx-auto w-full max-w-[min(100%,calc(100dvh-12rem))]">
                <div className="mb-3 flex flex-wrap items-center justify-between gap-2 rounded-md border bg-card px-3 py-2">
                  <div className="flex items-center gap-2">
                    <Badge variant={game.status === 'check' ? 'warning' : 'secondary'}>{turnLabel(game)}</Badge>
                    <span className="text-sm text-muted-foreground">You are {game.viewerSeat}</span>
                  </div>
                  <Button type="button" variant="ghost" size="sm" onClick={flipBoard}>
                    <RotateCcw />
                    Flip board
                  </Button>
                </div>
                {!game.players.black && (
                  <p className="mb-3 rounded-md border border-warning/40 bg-warning/5 p-3 text-sm">
                    Waiting for a Black player. Open this frontend in a different browser or private window,
                    start a separate guest session, and join with the game ID above.
                  </p>
                )}
                <ChessBoard
                  fen={game.fen || START_FEN}
                  orientation={orientation}
                  legalMoves={boardMoves}
                  lastMove={lastMove}
                  checkSquare={game.status === 'check' ? kingSquare(game.fen, getTurnFromFen(game.fen)) : null}
                  showCoordinates={coordinatesVisible}
                  interactive={!busy && Boolean(game.players.black) && game.viewerSeat === game.sideToMove && !isGameOver(game.status)}
                  onMove={handleMove}
                />
              </section>

              <aside className="space-y-4 rounded-lg border bg-card p-4">
                <div className="flex items-center justify-between gap-2">
                  <h2 className="font-semibold">Moves</h2>
                  <span className="text-xs text-muted-foreground">Version {game.version}</span>
                </div>
                {history.length === 0 ? (
                  <p className="text-sm text-muted-foreground">No moves yet.</p>
                ) : (
                  <ol className="max-h-96 space-y-1 overflow-y-auto" aria-label="Game moves">
                    {history.map((move, index) => (
                      <li key={move.ply} className="grid grid-cols-[2rem_1fr_1fr] gap-2 font-mono text-sm">
                        <span className="text-muted-foreground">{Math.ceil((index + 1) / 2)}.</span>
                        <span>{index % 2 === 0 ? move.san : ''}</span>
                        <span>{index % 2 === 1 ? move.san : ''}</span>
                      </li>
                    ))}
                  </ol>
                )}
                <Button type="button" variant="outline" className="w-full" disabled={busy || isGameOver(game.status)} onClick={() => void handleResign()}>
                  Resign
                </Button>
              </aside>
            </div>
          ) : (
            <section className="rounded-lg border border-dashed p-8 text-center">
              <h2 className="font-semibold">Create a game or join one</h2>
              <p className="mt-2 text-sm text-muted-foreground">The game server owns the position and validates every move.</p>
            </section>
          )}
        </>
      )}
    </PageTransition>
  );
}
