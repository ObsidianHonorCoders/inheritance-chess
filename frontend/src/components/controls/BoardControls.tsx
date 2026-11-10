import { FlipVertical2, Hash, RotateCcw } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { useGameStore } from '@/store/gameStore';
import { useSettingsStore } from '@/store/settingsStore';

export function BoardControls() {
  const flipBoard = useSettingsStore((state) => state.flipBoard);
  const coordinatesVisible = useSettingsStore((state) => state.coordinatesVisible);
  const setCoordinatesVisible = useSettingsStore((state) => state.setCoordinatesVisible);
  const resetGame = useGameStore((state) => state.resetGame);

  return (
    <div className="flex flex-wrap items-center justify-center gap-2" role="toolbar" aria-label="Board controls">
      <Button variant="secondary" size="sm" onClick={flipBoard}>
        <FlipVertical2 />
        Flip
      </Button>
      <Button
        variant="secondary"
        size="sm"
        aria-pressed={coordinatesVisible}
        onClick={() => setCoordinatesVisible(!coordinatesVisible)}
      >
        <Hash />
        Coordinates
      </Button>
      <Button variant="secondary" size="sm" onClick={resetGame}>
        <RotateCcw />
        Reset
      </Button>
    </div>
  );
}
