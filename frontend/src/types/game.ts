import type { Color, PieceSymbol, Square } from './chess';

export type PromotionPiece = Extract<PieceSymbol, 'q' | 'r' | 'b' | 'n'>;

export interface MoveInput {
  from: Square;
  to: Square;
  promotion?: PromotionPiece;
}

export interface LegalMove {
  from: Square;
  to: Square;
  san: string;
  promotion?: PieceSymbol;
  isCapture: boolean;
}

export interface MoveRecord {
  /** 1-based index in the move list. */
  ply: number;
  /** Full-move number as defined by FEN. */
  moveNumber: number;
  color: Color;
  piece: PieceSymbol;
  from: Square;
  to: Square;
  san: string;
  uci: string;
  captured?: PieceSymbol;
  promotion?: PieceSymbol;
  /** Domain move flags used for sound and board annotations. */
  flags: string;
  fenAfter: string;
}

export interface CapturedPieces {
  /** Black pieces captured by White. */
  byWhite: PieceSymbol[];
  /** White pieces captured by Black. */
  byBlack: PieceSymbol[];
}

export type GameEndReason =
  | 'checkmate'
  | 'stalemate'
  | 'insufficient-material'
  | 'threefold-repetition'
  | 'fifty-move-rule';

export interface GameStatus {
  isCheck: boolean;
  isCheckmate: boolean;
  isStalemate: boolean;
  isDraw: boolean;
  isGameOver: boolean;
  /** Square of the king currently in check. */
  checkSquare: Square | null;
  outcome: 'white' | 'black' | 'draw' | null;
  reason: GameEndReason | null;
}

/** Everything derived from (startFen, history, currentPly). */
export interface GameState {
  startFen: string;
  history: MoveRecord[];
  /** Number of plies applied in the displayed position (0..history.length). */
  currentPly: number;
  fen: string;
  turn: Color;
  legalMoves: LegalMove[];
  pgn: string;
  capturedPieces: CapturedPieces;
  status: GameStatus;
  lastMove: MoveRecord | null;
}
