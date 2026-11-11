export type BoardThemeId = 'classic' | 'wood' | 'dark' | 'green' | 'blue' | 'tournament';

export type PieceThemeId = 'alpha' | 'merida' | 'neo' | 'staunton';

export interface BoardTheme {
  id: BoardThemeId;
  name: string;
  lightSquare: string;
  darkSquare: string;
  selection: string;
  moveHighlight: string;
}

export interface PieceTheme {
  id: PieceThemeId;
  name: string;
  description: string;
}
