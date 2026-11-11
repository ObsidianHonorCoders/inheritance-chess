import { PieceImage } from '@/components/chessboard/PieceImage';
import { RadioGroup } from '@/components/ui/radio-group';
import { useBoardTheme, usePieceTheme } from '@/features/themes';
import type { PieceCode, PieceThemeId } from '@/types';
import { ThemeOptionCard } from './ThemeOptionCard';

const PREVIEW_PIECES: readonly PieceCode[] = ['wK', 'bQ', 'wN', 'bP'];

function PieceThemePreview({ themeId }: { themeId: PieceThemeId }) {
  const { theme } = useBoardTheme();

  return (
    <div className="grid grid-cols-4 overflow-hidden rounded-md" aria-hidden>
      {PREVIEW_PIECES.map((code, index) => (
        <div
          key={code}
          className="aspect-square"
          style={{ backgroundColor: index % 2 === 0 ? theme.lightSquare : theme.darkSquare }}
        >
          <PieceImage code={code} themeId={themeId} className="h-full w-full" />
        </div>
      ))}
    </div>
  );
}

export function PieceThemeSelector() {
  const { theme, themes, setTheme } = usePieceTheme();

  return (
    <RadioGroup
      value={theme.id}
      onValueChange={(value) => setTheme(value as PieceThemeId)}
      aria-label="Piece theme"
      className="grid-cols-1 gap-3 sm:grid-cols-2"
    >
      {themes.map((option) => (
        <ThemeOptionCard
          key={option.id}
          value={option.id}
          label={option.name}
          selected={option.id === theme.id}
          indicatorId="piece-theme-indicator"
        >
          <PieceThemePreview themeId={option.id} />
        </ThemeOptionCard>
      ))}
    </RadioGroup>
  );
}
