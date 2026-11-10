import { Badge } from '@/components/ui/badge';
import { useGameStore } from '@/store/gameStore';
import { cn } from '@/utils/cn';
import { describeGameStatus } from '@/utils/format';

export function GameStatusBanner() {
  const status = useGameStore((state) => state.status);
  const turn = useGameStore((state) => state.turn);

  return (
    <div
      role="status"
      aria-live="polite"
      className="mb-3 flex items-center justify-between gap-2 rounded-md border bg-card px-3 py-2"
    >
      <div className="flex items-center gap-2 text-sm font-medium">
        <span
          aria-hidden
          className={cn(
            'size-3 rounded-full border',
            turn === 'w' ? 'bg-zinc-100' : 'bg-zinc-900',
            status.isGameOver && 'opacity-40',
          )}
        />
        {describeGameStatus(status, turn)}
      </div>
      {status.isGameOver && <Badge variant="outline">Game over</Badge>}
    </div>
  );
}
