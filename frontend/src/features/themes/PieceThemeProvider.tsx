import { createContext, useContext, useMemo, type ReactElement, type ReactNode } from 'react';
import { useSettingsStore } from '@/store/settingsStore';
import type { PieceCode, PieceTheme, PieceThemeId } from '@/types';
import { getPieceTheme, getPieceUrl, PIECE_CODES, PIECE_THEMES } from './pieceThemes';

/** Shape accepted by react-chessboard's `customPieces` prop. */
export type CustomPieces = Partial<
  Record<PieceCode, (args: { squareWidth: number }) => ReactElement>
>;

interface PieceThemeContextValue {
  theme: PieceTheme;
  themes: readonly PieceTheme[];
  setTheme: (id: PieceThemeId) => void;
  getPieceUrl: (code: PieceCode) => string;
  /** Memoized per theme so switching sets never touches board logic. */
  customPieces: CustomPieces;
}

const PieceThemeContext = createContext<PieceThemeContextValue | null>(null);

export function PieceThemeProvider({ children }: { children: ReactNode }) {
  const themeId = useSettingsStore((state) => state.pieceTheme);
  const setTheme = useSettingsStore((state) => state.setPieceTheme);

  const value = useMemo<PieceThemeContextValue>(() => {
    const customPieces: CustomPieces = {};
    for (const code of PIECE_CODES) {
      const url = getPieceUrl(themeId, code);
      customPieces[code] = ({ squareWidth }) => (
        <img
          src={url}
          alt=""
          draggable={false}
          width={squareWidth}
          height={squareWidth}
          style={{ display: 'block', width: squareWidth, height: squareWidth }}
        />
      );
    }

    return {
      theme: getPieceTheme(themeId),
      themes: PIECE_THEMES,
      setTheme,
      getPieceUrl: (code) => getPieceUrl(themeId, code),
      customPieces,
    };
  }, [themeId, setTheme]);

  return <PieceThemeContext.Provider value={value}>{children}</PieceThemeContext.Provider>;
}

export function usePieceTheme(): PieceThemeContextValue {
  const context = useContext(PieceThemeContext);
  if (!context) throw new Error('usePieceTheme must be used within a PieceThemeProvider');
  return context;
}
