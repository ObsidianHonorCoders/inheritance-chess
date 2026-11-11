import { RadioGroup } from '@/components/ui/radio-group';
import { useBoardTheme } from '@/features/themes';
import type { BoardTheme, BoardThemeId } from '@/types';
import { ThemeOptionCard } from './ThemeOptionCard';

function BoardThemePreview({ theme }: { theme: BoardTheme }) {
  return (
    <div className="grid aspect-[2/1] w-full grid-cols-4 overflow-hidden rounded-md" aria-hidden>
      {Array.from({ length: 8 }, (_, index) => {
        const isLight = (Math.floor(index / 4) + index) % 2 === 0;
        return (
          <div
            key={index}
            style={{
              backgroundColor: isLight ? theme.lightSquare : theme.darkSquare,
              boxShadow: index === 5 ? `inset 0 0 0 100px ${theme.moveHighlight}` : undefined,
            }}
          />
        );
      })}
    </div>
  );
}

export function BoardThemeSelector() {
  const { theme, themes, setTheme } = useBoardTheme();

  return (
    <RadioGroup
      value={theme.id}
      onValueChange={(value) => setTheme(value as BoardThemeId)}
      aria-label="Board theme"
      className="grid-cols-2 gap-3 sm:grid-cols-3"
    >
      {themes.map((option) => (
        <ThemeOptionCard
          key={option.id}
          value={option.id}
          label={option.name}
          selected={option.id === theme.id}
          indicatorId="board-theme-indicator"
        >
          <BoardThemePreview theme={option} />
        </ThemeOptionCard>
      ))}
    </RadioGroup>
  );
}
