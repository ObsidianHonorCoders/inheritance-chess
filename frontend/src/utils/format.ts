import type { Color, GameStatus } from '@/types';

/** Formats a White-perspective evaluation in pawns, e.g. `+0.45`. */
export function formatEvaluation(evaluation: number | null): string {
  if (evaluation === null) return '–';
  const rounded = Math.abs(evaluation) < 0.005 ? 0 : evaluation;
  return `${rounded > 0 ? '+' : ''}${rounded.toFixed(2)}`;
}

/** Compact label for the evaluation bar (absolute value, one decimal). */
export function formatEvaluationShort(evaluation: number | null): string {
  if (evaluation === null) return '–';
  return Math.abs(evaluation).toFixed(1);
}

export function formatNodes(nodes: number | null): string {
  if (nodes === null) return '–';
  if (nodes >= 1e9) return `${(nodes / 1e9).toFixed(2)}B`;
  if (nodes >= 1e6) return `${(nodes / 1e6).toFixed(2)}M`;
  if (nodes >= 1e3) return `${(nodes / 1e3).toFixed(1)}K`;
  return String(nodes);
}

/** Maps an evaluation in pawns to White's share of the bar (2..98 %). */
export function evaluationToWhitePercent(evaluation: number | null): number {
  if (evaluation === null) return 50;
  const share = 100 / (1 + Math.exp(-0.7 * evaluation));
  return Math.min(98, Math.max(2, share));
}

export function describeGameStatus(status: GameStatus, turn: Color): string {
  switch (status.reason) {
    case 'checkmate':
      return `Checkmate — ${status.outcome === 'white' ? 'White' : 'Black'} wins`;
    case 'stalemate':
      return 'Draw by stalemate';
    case 'insufficient-material':
      return 'Draw by insufficient material';
    case 'threefold-repetition':
      return 'Draw by threefold repetition';
    case 'fifty-move-rule':
      return 'Draw by fifty-move rule';
    case null:
      return `${turn === 'w' ? 'White' : 'Black'} to move${status.isCheck ? ' — check' : ''}`;
  }
}
