import { createContext, useContext, useMemo, type ReactNode } from 'react';
import { useSettingsStore } from '@/store/settingsStore';
import type { BoardTheme, BoardThemeId } from '@/types';
import { BOARD_THEMES, getBoardTheme } from './boardThemes';

interface BoardThemeContextValue {
  theme: BoardTheme;
  themes: readonly BoardTheme[];
  setTheme: (id: BoardThemeId) => void;
}

const BoardThemeContext = createContext<BoardThemeContextValue | null>(null);

export function BoardThemeProvider({ children }: { children: ReactNode }) {
  const themeId = useSettingsStore((state) => state.boardTheme);
  const setTheme = useSettingsStore((state) => state.setBoardTheme);

  const value = useMemo<BoardThemeContextValue>(
    () => ({ theme: getBoardTheme(themeId), themes: BOARD_THEMES, setTheme }),
    [themeId, setTheme],
  );

  return <BoardThemeContext.Provider value={value}>{children}</BoardThemeContext.Provider>;
}

export function useBoardTheme(): BoardThemeContextValue {
  const context = useContext(BoardThemeContext);
  if (!context) throw new Error('useBoardTheme must be used within a BoardThemeProvider');
  return context;
}
