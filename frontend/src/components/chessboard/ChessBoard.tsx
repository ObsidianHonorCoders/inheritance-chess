import { useCallback, useEffect, useMemo, useState } from 'react';
import { Chessboard } from 'react-chessboard';
import { useBoardTheme, usePieceTheme } from '@/features/themes';
import { useAnimationsEnabled, useElementWidth } from '@/hooks';
import type { BoardOrientation, LegalMove, MoveInput, PromotionPiece, Square } from '@/types';
import { cn } from '@/utils/cn';
import { getTurnFromFen } from '@/utils/fen';
import { PromotionDialog } from './PromotionDialog';

// TODO(V3): engine arrows & candidate-move overlays via react-chessboard's `customArrows`.
// TODO(V2): captured pieces / clocks rendered in player strips above and below the board.

const MOVE_DOT = 'radial-gradient(circle, rgba(0, 0, 0, 0.28) 0%, rgba(0, 0, 0, 0.28) 22%, transparent 24%)';
const CAPTURE_RING = 'radial-gradient(circle, transparent 0%, transparent 78%, rgba(0, 0, 0, 0.25) 80%)';
const CHECK_GLOW =
  'radial-gradient(circle, rgb(255, 0, 0) 0%, rgb(231, 0, 0) 25%, rgba(169, 0, 0, 0) 89%, rgba(158, 0, 0, 0) 100%)';

type StyleRecord = Record<string, string | number>;

const BOARD_STYLE: StyleRecord = {
  borderRadius: '6px',
  boxShadow: '0 8px 30px rgba(0, 0, 0, 0.45)',
  overflow: 'hidden',
};

type SquareStyles = Partial<Record<Square, StyleRecord>>;

const neverPromoteInternally = (): boolean => false;

const NO_MOVES: readonly LegalMove[] = [];

export interface ChessBoardProps {
  fen: string;
  orientation?: BoardOrientation;
  legalMoves?: readonly LegalMove[];
  lastMove?: { from: Square; to: Square } | null;
  checkSquare?: Square | null;
  showCoordinates?: boolean;
  /** Disables interaction (drag, click-to-move) for previews. */
  interactive?: boolean;
  /** Return `true` if the move was applied. */
  onMove?: (move: MoveInput) => boolean;
  /** Unique id; required when several boards are mounted at once. */
  boardId?: string;
  className?: string;
}

export function ChessBoard({
  fen,
  orientation = 'white',
  legalMoves = NO_MOVES,
  lastMove = null,
  checkSquare = null,
  showCoordinates = true,
  interactive = true,
  onMove,
  boardId = 'main-board',
  className,
}: ChessBoardProps) {
  const { theme } = useBoardTheme();
  const { customPieces } = usePieceTheme();
  const animationsEnabled = useAnimationsEnabled();
  const [containerRef, width] = useElementWidth<HTMLDivElement>();

  const [selected, setSelected] = useState<Square | null>(null);
  const [pendingPromotion, setPendingPromotion] = useState<{ from: Square; to: Square } | null>(null);

  const turn = getTurnFromFen(fen);

  // A new position invalidates any selection or pending promotion.
  useEffect(() => {
    setSelected(null);
    setPendingPromotion(null);
  }, [fen]);

  const movableSquares = useMemo(() => new Set(legalMoves.map((move) => move.from)), [legalMoves]);

  const targets = useMemo(() => {
    const map = new Map<Square, boolean>();
    if (!selected) return map;
    for (const move of legalMoves) {
      if (move.from === selected) map.set(move.to, (map.get(move.to) ?? false) || move.isCapture);
    }
    return map;
  }, [legalMoves, selected]);

  const squareStyles = useMemo<SquareStyles>(() => {
    const styles: SquareStyles = {};
    const patch = (square: Square, style: StyleRecord) => {
      styles[square] = { ...styles[square], ...style };
    };

    if (lastMove) {
      patch(lastMove.from, { backgroundColor: theme.moveHighlight });
      patch(lastMove.to, { backgroundColor: theme.moveHighlight });
    }
    if (selected) patch(selected, { backgroundColor: theme.selection });
    targets.forEach((isCapture, square) => {
      patch(square, { backgroundImage: isCapture ? CAPTURE_RING : MOVE_DOT });
    });
    if (checkSquare) patch(checkSquare, { backgroundImage: CHECK_GLOW });

    return styles;
  }, [lastMove, selected, targets, checkSquare, theme.moveHighlight, theme.selection]);

  const attemptMove = useCallback(
    (from: Square, to: Square): boolean => {
      if (!interactive || !onMove) return false;

      const candidates = legalMoves.filter((move) => move.from === from && move.to === to);
      if (candidates.length === 0) return false;

      if (candidates.some((move) => move.promotion)) {
        setPendingPromotion({ from, to });
        return false;
      }

      const applied = onMove({ from, to });
      if (applied) setSelected(null);
      return applied;
    },
    [interactive, onMove, legalMoves],
  );

  const handlePieceDrop = useCallback(
    (from: string, to: string): boolean => attemptMove(from as Square, to as Square),
    [attemptMove],
  );

  const handleDragBegin = useCallback(
    (_piece: string, square: string) => {
      setSelected(movableSquares.has(square as Square) ? (square as Square) : null);
    },
    [movableSquares],
  );

  const handleSquareClick = useCallback(
    (square: string) => {
      if (!interactive) return;
      const clicked = square as Square;

      if (selected && selected !== clicked && targets.has(clicked)) {
        attemptMove(selected, clicked);
        return;
      }
      setSelected(selected === clicked || !movableSquares.has(clicked) ? null : clicked);
    },
    [interactive, selected, targets, movableSquares, attemptMove],
  );

  const isDraggablePiece = useCallback(
    ({ piece }: { piece: string }): boolean => interactive && piece.startsWith(turn),
    [interactive, turn],
  );

  const handlePromotionSelect = (promotion: PromotionPiece) => {
    if (pendingPromotion && onMove) onMove({ ...pendingPromotion, promotion });
    setPendingPromotion(null);
    setSelected(null);
  };

  return (
    <div
      ref={containerRef}
      role="group"
      aria-label="Chess board"
      className={cn('relative aspect-square w-full select-none', className)}
    >
      {width > 0 && (
        <Chessboard
          id={boardId}
          position={fen}
          boardWidth={width}
          boardOrientation={orientation}
          showBoardNotation={showCoordinates}
          animationDuration={animationsEnabled ? 200 : 0}
          arePiecesDraggable={interactive}
          isDraggablePiece={isDraggablePiece}
          onPieceDrop={handlePieceDrop}
          onPieceDragBegin={handleDragBegin}
          onSquareClick={handleSquareClick}
          onPromotionCheck={neverPromoteInternally}
          customPieces={customPieces}
          customSquareStyles={squareStyles}
          customDarkSquareStyle={{ backgroundColor: theme.darkSquare }}
          customLightSquareStyle={{ backgroundColor: theme.lightSquare }}
          customBoardStyle={BOARD_STYLE}
        />
      )}
      <PromotionDialog
        open={pendingPromotion !== null}
        color={turn}
        onSelect={handlePromotionSelect}
        onCancel={() => setPendingPromotion(null)}
      />
    </div>
  );
}
