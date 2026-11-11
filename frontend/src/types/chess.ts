export type Color = 'w' | 'b';
export type PieceSymbol = 'p' | 'n' | 'b' | 'r' | 'q' | 'k';
export type Square =
  | `${'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'g' | 'h'}${1 | 2 | 3 | 4 | 5 | 6 | 7 | 8}`;

export type BoardOrientation = 'white' | 'black';

/** Piece identifier used by piece-set assets, e.g. `wK` or `bP`. */
export type PieceCode = `${Color}${'P' | 'N' | 'B' | 'R' | 'Q' | 'K'}`;
