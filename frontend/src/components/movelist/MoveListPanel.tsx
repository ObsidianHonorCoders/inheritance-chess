import { useEffect, useMemo, useRef, type RefObject } from 'react';
import { ChevronLeft, ChevronRight, ChevronsLeft, ChevronsRight } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { ScrollArea } from '@/components/ui/scroll-area';
import { useGameStore } from '@/store/gameStore';
import type { MoveRecord } from '@/types';
import { cn } from '@/utils/cn';
import { buildMoveRows } from '@/utils/moves';

interface MoveButtonProps {
  record: MoveRecord | undefined;
  active: boolean;
  activeRef: RefObject<HTMLButtonElement>;
  onSelect: (ply: number) => void;
}

function MoveButton({ record, active, activeRef, onSelect }: MoveButtonProps) {
  if (!record) return <span className="px-2 text-muted-foreground">…</span>;

  return (
    <button
      type="button"
      ref={active ? activeRef : undefined}
      onClick={() => onSelect(record.ply)}
      aria-current={active ? 'true' : undefined}
      className={cn(
        'rounded px-2 py-1 text-left font-mono text-sm transition-colors hover:bg-accent focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring',
        active && 'bg-primary/20 font-semibold text-primary',
      )}
    >
      {record.san}
    </button>
  );
}

export function MoveListPanel() {
  const history = useGameStore((state) => state.history);
  const currentPly = useGameStore((state) => state.currentPly);
  const goToPly = useGameStore((state) => state.goToPly);
  const goToStart = useGameStore((state) => state.goToStart);
  const goToPrevious = useGameStore((state) => state.goToPrevious);
  const goToNext = useGameStore((state) => state.goToNext);
  const goToEnd = useGameStore((state) => state.goToEnd);

  const rows = useMemo(() => buildMoveRows(history), [history]);
  const activeRef = useRef<HTMLButtonElement>(null);

  useEffect(() => {
    activeRef.current?.scrollIntoView({ block: 'nearest' });
  }, [currentPly, history.length]);

  const atStart = currentPly === 0;
  const atEnd = currentPly === history.length;

  return (
    <div className="space-y-3">
      <div className="flex items-center justify-center gap-1" role="toolbar" aria-label="Move navigation">
        <Button variant="outline" size="icon" aria-label="First move" disabled={atStart} onClick={goToStart}>
          <ChevronsLeft />
        </Button>
        <Button variant="outline" size="icon" aria-label="Previous move" disabled={atStart} onClick={goToPrevious}>
          <ChevronLeft />
        </Button>
        <Button variant="outline" size="icon" aria-label="Next move" disabled={atEnd} onClick={goToNext}>
          <ChevronRight />
        </Button>
        <Button variant="outline" size="icon" aria-label="Last move" disabled={atEnd} onClick={goToEnd}>
          <ChevronsRight />
        </Button>
      </div>

      <ScrollArea className="h-56 rounded-md border bg-secondary/20">
        {rows.length === 0 ? (
          <p className="p-4 text-center text-sm text-muted-foreground">No moves yet</p>
        ) : (
          <ol className="p-1" aria-label="Moves">
            {rows.map((row) => (
              <li
                key={`${row.moveNumber}-${row.white?.ply ?? row.black?.ply}`}
                className="grid grid-cols-[2.5rem_1fr_1fr] items-center"
              >
                <span className="pl-2 font-mono text-xs text-muted-foreground">{row.moveNumber}.</span>
                <MoveButton
                  record={row.white}
                  active={row.white?.ply === currentPly}
                  activeRef={activeRef}
                  onSelect={goToPly}
                />
                {row.black ? (
                  <MoveButton
                    record={row.black}
                    active={row.black.ply === currentPly}
                    activeRef={activeRef}
                    onSelect={goToPly}
                  />
                ) : (
                  <span />
                )}
              </li>
            ))}
          </ol>
        )}
      </ScrollArea>
    </div>
  );
}
