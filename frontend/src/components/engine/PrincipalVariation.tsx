import { useMemo } from 'react';
import { uciToSanLine } from '@/utils/chess';
import { cn } from '@/utils/cn';

interface PrincipalVariationProps {
  /** Position the line starts from. */
  fen: string | null;
  /** Moves in UCI notation. */
  moves: readonly string[];
  maxMoves?: number;
  className?: string;
}

export function PrincipalVariation({ fen, moves, maxMoves, className }: PrincipalVariationProps) {
  const entries = useMemo(() => {
    if (!fen) return [];
    const line = uciToSanLine(fen, moves);
    return maxMoves ? line.slice(0, maxMoves) : line;
  }, [fen, moves, maxMoves]);

  if (entries.length === 0) {
    return <p className={cn('text-sm text-muted-foreground', className)}>No line available</p>;
  }

  return (
    <p className={cn('flex flex-wrap items-baseline gap-x-1.5 gap-y-0.5 font-mono text-sm', className)}>
      {entries.map((entry, index) => (
        <span key={`${index}-${entry.san}`} className="inline-flex items-baseline gap-1">
          {entry.label && <span className="text-muted-foreground">{entry.label}</span>}
          <span className="font-medium">{entry.san}</span>
        </span>
      ))}
    </p>
  );
}
