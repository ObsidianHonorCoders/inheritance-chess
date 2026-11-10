import { validateFen } from 'chess.js';
import type { Color, OperationResult } from '@/types';

export function validateFenString(fen: string): OperationResult {
  const trimmed = fen.trim();
  if (!trimmed) return { ok: false, error: 'FEN is empty.' };

  const result = validateFen(trimmed);
  return result.ok ? { ok: true } : { ok: false, error: result.error ?? 'Invalid FEN.' };
}

export function getTurnFromFen(fen: string): Color {
  return fen.split(' ')[1] === 'b' ? 'b' : 'w';
}
