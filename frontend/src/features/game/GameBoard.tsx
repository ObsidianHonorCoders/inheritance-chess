import { useCallback } from 'react';
import { ChessBoard } from '@/components/chessboard/ChessBoard';
import { useGameStore } from '@/store/gameStore';
import { useSettingsStore } from '@/store/settingsStore';
import type { MoveInput } from '@/types';
import { useGameActions } from './useGameActions';

interface GameBoardProps {
  interactive?: boolean;
  className?: string;
}

/** ChessBoard wired to the game and settings stores. */
export function GameBoard({ interactive = true, className }: GameBoardProps) {
  const fen = useGameStore((state) => state.fen);
  const legalMoves = useGameStore((state) => state.legalMoves);
  const lastMove = useGameStore((state) => state.lastMove);
  const checkSquare = useGameStore((state) => state.status.checkSquare);
  const orientation = useSettingsStore((state) => state.boardOrientation);
  const showCoordinates = useSettingsStore((state) => state.coordinatesVisible);
  const { playMove } = useGameActions();

  const handleMove = useCallback((move: MoveInput) => playMove(move) !== null, [playMove]);

  return (
    <ChessBoard
      fen={fen}
      orientation={orientation}
      legalMoves={legalMoves}
      lastMove={lastMove}
      checkSquare={checkSquare}
      showCoordinates={showCoordinates}
      interactive={interactive}
      onMove={handleMove}
      className={className}
    />
  );
}
