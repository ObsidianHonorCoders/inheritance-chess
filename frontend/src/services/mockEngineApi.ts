import { Chess, type Move, type PieceSymbol } from 'chess.js';
import type { EngineAnalysis, EngineApi } from '@/types';
import { START_FEN } from '@/utils/chess';

/** Canonical mock response, returned verbatim for the starting position. */
export const MOCK_ANALYSIS: EngineAnalysis = {
  bestMove: 'e2e4',
  evaluation: 0.45,
  depth: 18,
  nodes: 2_450_000,
  pv: ['e2e4', 'e7e5', 'g1f3'],
};

const MOCK_CONNECT_LATENCY_MS = 400;
const MOCK_ANALYZE_LATENCY_MS = 450;
const PIECE_VALUES: Record<PieceSymbol, number> = { p: 1, n: 3, b: 3, r: 5, q: 9, k: 0 };

function delay(ms: number, signal?: AbortSignal): Promise<void> {
  return new Promise((resolve, reject) => {
    if (signal?.aborted) {
      reject(new DOMException('Aborted', 'AbortError'));
      return;
    }
    const onAbort = (): void => {
      clearTimeout(timer);
      reject(new DOMException('Aborted', 'AbortError'));
    };
    const timer = setTimeout(() => {
      signal?.removeEventListener('abort', onAbort);
      resolve();
    }, ms);
    signal?.addEventListener('abort', onAbort, { once: true });
  });
}

function scoreMove(move: Move): number {
  let score = 0;
  if (move.captured) score += PIECE_VALUES[move.captured] * 10 - PIECE_VALUES[move.piece];
  if (move.promotion) score += 8;
  if (move.san.includes('#')) score += 1000;
  else if (move.san.includes('+')) score += 2;

  // Prefer central target squares to break ties.
  const file = move.to.charCodeAt(0) - 97;
  const rank = Number(move.to[1]) - 1;
  score += (3.5 - (Math.abs(file - 3.5) + Math.abs(rank - 3.5)) / 2) * 0.1;
  return score;
}

function pickMove(chess: Chess): Move | null {
  let best: Move | null = null;
  let bestScore = -Infinity;
  for (const move of chess.moves({ verbose: true })) {
    const score = scoreMove(move);
    if (score > bestScore) {
      best = move;
      bestScore = score;
    }
  }
  return best;
}

function materialBalance(chess: Chess): number {
  let balance = 0;
  for (const row of chess.board()) {
    for (const cell of row) {
      if (cell) balance += (cell.color === 'w' ? 1 : -1) * PIECE_VALUES[cell.type];
    }
  }
  return balance;
}

/** Produces a legal, position-aware fake analysis so the UI stays coherent after the first move. */
function deriveAnalysis(fen: string): EngineAnalysis {
  const chess = new Chess(fen);
  const turn = chess.turn();

  if (chess.isCheckmate()) {
    return { bestMove: null, evaluation: turn === 'w' ? -99 : 99, depth: 0, nodes: 0, pv: [] };
  }

  const evaluation = Math.round((materialBalance(chess) + (turn === 'w' ? 0.25 : -0.25)) * 100) / 100;
  const pv: string[] = [];
  for (let i = 0; i < 3; i++) {
    const move = pickMove(chess);
    if (!move) break;
    chess.move(move);
    pv.push(`${move.from}${move.to}${move.promotion ?? ''}`);
  }

  return { bestMove: pv[0] ?? null, evaluation, depth: MOCK_ANALYSIS.depth, nodes: MOCK_ANALYSIS.nodes, pv };
}

export const mockEngineApi: EngineApi = {
  async connect() {
    await delay(MOCK_CONNECT_LATENCY_MS);
  },

  async disconnect() {
    // Nothing to tear down for the mock.
  },

  async analyze({ fen, signal }) {
    await delay(MOCK_ANALYZE_LATENCY_MS, signal);
    if (fen === START_FEN) return { ...MOCK_ANALYSIS, pv: [...MOCK_ANALYSIS.pv] };

    try {
      return deriveAnalysis(fen);
    } catch {
      return { ...MOCK_ANALYSIS, pv: [...MOCK_ANALYSIS.pv] };
    }
  },
};
