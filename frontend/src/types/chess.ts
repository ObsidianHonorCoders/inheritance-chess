import type { Color, PieceSymbol, Square } from 'chess.js';

export type { Color, PieceSymbol, Square };

export type BoardOrientation = 'white' | 'black';

/** Piece identifier used by piece-set assets, e.g. `wK` or `bP`. */
export type PieceCode = `${Color}${'P' | 'N' | 'B' | 'R' | 'Q' | 'K'}`;
