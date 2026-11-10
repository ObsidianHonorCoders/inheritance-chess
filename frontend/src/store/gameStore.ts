import { create } from 'zustand';
import type { GameState, MoveInput, MoveRecord, OperationResult } from '@/types';
import { Chess } from 'chess.js';
import { applyMove, buildGameState, parsePgn, START_FEN } from '@/utils/chess';
import { errorMessage } from '@/utils/errors';
import { validateFenString } from '@/utils/fen';

// TODO(V2): clock state (per-side remaining time, increment, flag detection).
// TODO(V2): expose captured-piece material difference for the captured pieces visualization.
// TODO(V3): variations — replace "truncate on move" with a move tree.

export interface GameActions {
  /** Plays a move from the displayed position. Truncates any moves after it. */
  makeMove: (input: MoveInput) => MoveRecord | null;
  goToPly: (ply: number) => void;
  goToStart: () => void;
  goToPrevious: () => void;
  goToNext: () => void;
  goToEnd: () => void;
  loadFen: (fen: string) => OperationResult;
  loadPgn: (pgn: string) => OperationResult;
  resetGame: () => void;
}

export type GameStore = GameState & GameActions;

export const useGameStore = create<GameStore>()((set, get) => ({
  ...buildGameState(START_FEN, [], 0),

  makeMove: (input) => {
    const { fen, history, currentPly, startFen } = get();
    const record = applyMove(fen, input, currentPly + 1);
    if (!record) return null;

    const nextHistory = [...history.slice(0, currentPly), record];
    set(buildGameState(startFen, nextHistory, nextHistory.length));
    return record;
  },

  goToPly: (ply) => {
    const { startFen, history, currentPly } = get();
    const target = Math.min(Math.max(ply, 0), history.length);
    if (target === currentPly) return;
    set(buildGameState(startFen, history, target));
  },

  goToStart: () => get().goToPly(0),
  goToPrevious: () => get().goToPly(get().currentPly - 1),
  goToNext: () => get().goToPly(get().currentPly + 1),
  goToEnd: () => get().goToPly(get().history.length),

  loadFen: (fen) => {
    const validation = validateFenString(fen);
    if (!validation.ok) return validation;

    try {
      const normalized = new Chess(fen.trim()).fen();
      set(buildGameState(normalized, [], 0));
      return { ok: true };
    } catch (error) {
      return { ok: false, error: errorMessage(error) };
    }
  },

  loadPgn: (pgn) => {
    const parsed = parsePgn(pgn);
    if (!parsed.ok) return parsed;

    try {
      set(buildGameState(parsed.startFen, parsed.history, parsed.history.length));
      return { ok: true };
    } catch (error) {
      return { ok: false, error: errorMessage(error) };
    }
  },

  resetGame: () => set(buildGameState(START_FEN, [], 0)),
}));
