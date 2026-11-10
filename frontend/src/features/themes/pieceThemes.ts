import type { PieceCode, PieceTheme, PieceThemeId } from '@/types';

// TODO(V2): advanced piece sets (more families, user-uploaded sets, per-set board offsets).

export const PIECE_THEMES: readonly PieceTheme[] = [
  { id: 'alpha', name: 'Alpha', description: 'Thin diagram-style outlines.' },
  { id: 'merida', name: 'Merida', description: 'Classic tournament look.' },
  { id: 'neo', name: 'Neo', description: 'Flat and modern.' },
  { id: 'staunton', name: 'Staunton', description: 'Warm gradient ivory and ebony.' },
];

export const DEFAULT_PIECE_THEME_ID: PieceThemeId = 'merida';

export const PIECE_CODES: readonly PieceCode[] = [
  'wP', 'wN', 'wB', 'wR', 'wQ', 'wK',
  'bP', 'bN', 'bB', 'bR', 'bQ', 'bK',
];

export function getPieceTheme(id: PieceThemeId): PieceTheme {
  return PIECE_THEMES.find((theme) => theme.id === id) ?? PIECE_THEMES[0];
}

/** Piece sets live in `public/pieces/<set>/<code>.svg`. */
export function getPieceUrl(themeId: PieceThemeId, code: PieceCode): string {
  return `${import.meta.env.BASE_URL}pieces/${themeId}/${code}.svg`;
}
