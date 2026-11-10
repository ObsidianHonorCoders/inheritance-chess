import { Chess, DEFAULT_POSITION } from 'chess.js';
import type { Move } from 'chess.js';
import type {
  CapturedPieces,
  Color,
  GameState,
  GameStatus,
  LegalMove,
  MoveInput,
  MoveRecord,
  Square,
} from '@/types';
import { errorMessage } from './errors';

export const START_FEN: string = DEFAULT_POSITION;

type GameView = Pick<GameState, 'fen' | 'turn' | 'legalMoves' | 'status'>;

export function toMoveRecord(move: Move, ply: number): MoveRecord {
  return {
    ply,
    moveNumber: Number(move.before.split(' ')[5]) || 1,
    color: move.color,
    piece: move.piece,
    from: move.from,
    to: move.to,
    san: move.san,
    uci: `${move.from}${move.to}${move.promotion ?? ''}`,
    captured: move.captured,
    promotion: move.promotion,
    flags: move.flags,
    fenAfter: move.after,
  };
}

/** Applies a move on top of `fen`. Returns `null` when the move is illegal. */
export function applyMove(fen: string, input: MoveInput, ply: number): MoveRecord | null {
  try {
    const chess = new Chess(fen);
    const move = chess.move({ from: input.from, to: input.to, promotion: input.promotion });
    return toMoveRecord(move, ply);
  } catch {
    return null;
  }
}

function findKingSquare(chess: Chess, color: Color): Square | null {
  for (const row of chess.board()) {
    for (const cell of row) {
      if (cell && cell.type === 'k' && cell.color === color) return cell.square;
    }
  }
  return null;
}

function readStatus(chess: Chess): GameStatus {
  const turn = chess.turn();
  const isCheck = chess.isCheck();
  const isCheckmate = chess.isCheckmate();
  const isStalemate = chess.isStalemate();
  const isDraw = chess.isDraw();

  let reason: GameStatus['reason'] = null;
  if (isCheckmate) reason = 'checkmate';
  else if (isStalemate) reason = 'stalemate';
  else if (chess.isInsufficientMaterial()) reason = 'insufficient-material';
  else if (chess.isThreefoldRepetition()) reason = 'threefold-repetition';
  else if (chess.isDrawByFiftyMoves()) reason = 'fifty-move-rule';

  let outcome: GameStatus['outcome'] = null;
  if (isCheckmate) outcome = turn === 'w' ? 'black' : 'white';
  else if (isDraw) outcome = 'draw';

  return {
    isCheck,
    isCheckmate,
    isStalemate,
    isDraw,
    isGameOver: chess.isGameOver(),
    checkSquare: isCheck ? findKingSquare(chess, turn) : null,
    outcome,
    reason,
  };
}

function readView(chess: Chess): GameView {
  const legalMoves: LegalMove[] = chess.moves({ verbose: true }).map((move) => ({
    from: move.from,
    to: move.to,
    san: move.san,
    promotion: move.promotion,
    isCapture: Boolean(move.captured),
  }));

  return { fen: chess.fen(), turn: chess.turn(), legalMoves, status: readStatus(chess) };
}

function collectCaptured(history: readonly MoveRecord[], ply: number): CapturedPieces {
  const captured: CapturedPieces = { byWhite: [], byBlack: [] };
  for (let i = 0; i < ply; i++) {
    const record = history[i];
    if (!record.captured) continue;
    (record.color === 'w' ? captured.byWhite : captured.byBlack).push(record.captured);
  }
  return captured;
}

/** Rebuilds the full game state by replaying `history` from `startFen`. */
export function buildGameState(
  startFen: string,
  history: MoveRecord[],
  ply: number,
): GameState {
  const target = Math.min(Math.max(ply, 0), history.length);
  const chess = new Chess(startFen);

  let view: GameView | null = target === 0 ? readView(chess) : null;
  for (let i = 0; i < history.length; i++) {
    const record = history[i];
    chess.move({ from: record.from, to: record.to, promotion: record.promotion });
    if (i + 1 === target) view = readView(chess);
  }

  return {
    startFen,
    history,
    currentPly: target,
    ...(view ?? readView(chess)),
    pgn: chess.pgn(),
    capturedPieces: collectCaptured(history, target),
    lastMove: target > 0 ? history[target - 1] : null,
  };
}

export type ParsedPgn =
  | { ok: true; startFen: string; history: MoveRecord[] }
  | { ok: false; error: string };

export function parsePgn(pgn: string): ParsedPgn {
  const text = pgn.trim();
  if (!text) return { ok: false, error: 'PGN is empty.' };

  try {
    const chess = new Chess();
    chess.loadPgn(text);
    const moves = chess.history({ verbose: true });
    const startFen = moves.length > 0 ? moves[0].before : chess.fen();
    return {
      ok: true,
      startFen,
      history: moves.map((move, index) => toMoveRecord(move, index + 1)),
    };
  } catch (error) {
    return { ok: false, error: errorMessage(error) };
  }
}

export interface SanLineEntry {
  san: string;
  color: Color;
  /** Move-number label ("1." / "1...") shown before the move, if any. */
  label: string | null;
}

/** Converts a UCI line to SAN, stopping at the first move that is illegal in `fen`. */
export function uciToSanLine(fen: string, uciMoves: readonly string[]): SanLineEntry[] {
  let chess: Chess;
  try {
    chess = new Chess(fen);
  } catch {
    return [];
  }

  const entries: SanLineEntry[] = [];
  for (const uci of uciMoves) {
    const color = chess.turn();
    const moveNumber = Number(chess.fen().split(' ')[5]) || 1;
    try {
      const move = chess.move({
        from: uci.slice(0, 2),
        to: uci.slice(2, 4),
        promotion: uci.length > 4 ? uci.slice(4, 5) : undefined,
      });
      let label: string | null = null;
      if (color === 'w') label = `${moveNumber}.`;
      else if (entries.length === 0) label = `${moveNumber}...`;
      entries.push({ san: move.san, color, label });
    } catch {
      break;
    }
  }
  return entries;
}
