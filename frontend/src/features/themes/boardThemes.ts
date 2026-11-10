import type { BoardTheme, BoardThemeId } from '@/types';

// TODO(V2): advanced board themes (textured/image boards, custom user colors, per-theme border styles).

export const BOARD_THEMES: readonly BoardTheme[] = [
  {
    id: 'classic',
    name: 'Classic',
    lightSquare: '#f0d9b5',
    darkSquare: '#b58863',
    selection: 'rgba(20, 85, 30, 0.5)',
    moveHighlight: 'rgba(155, 199, 0, 0.41)',
  },
  {
    id: 'wood',
    name: 'Wood',
    lightSquare: '#e3c08d',
    darkSquare: '#9b6a3f',
    selection: 'rgba(250, 204, 21, 0.45)',
    moveHighlight: 'rgba(255, 221, 87, 0.4)',
  },
  {
    id: 'dark',
    name: 'Dark',
    lightSquare: '#7b8394',
    darkSquare: '#3f4656',
    selection: 'rgba(56, 189, 248, 0.45)',
    moveHighlight: 'rgba(56, 189, 248, 0.28)',
  },
  {
    id: 'green',
    name: 'Green',
    lightSquare: '#eeeed2',
    darkSquare: '#769656',
    selection: 'rgba(255, 255, 51, 0.5)',
    moveHighlight: 'rgba(255, 255, 51, 0.4)',
  },
  {
    id: 'blue',
    name: 'Blue',
    lightSquare: '#dee3e6',
    darkSquare: '#8ca2ad',
    selection: 'rgba(20, 85, 160, 0.5)',
    moveHighlight: 'rgba(30, 110, 200, 0.35)',
  },
  {
    id: 'tournament',
    name: 'Tournament',
    lightSquare: '#f5f5dc',
    darkSquare: '#2e7d5b',
    selection: 'rgba(234, 179, 8, 0.55)',
    moveHighlight: 'rgba(234, 179, 8, 0.38)',
  },
];

export const DEFAULT_BOARD_THEME_ID: BoardThemeId = 'classic';

export function getBoardTheme(id: BoardThemeId): BoardTheme {
  return BOARD_THEMES.find((theme) => theme.id === id) ?? BOARD_THEMES[0];
}
