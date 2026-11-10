import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import type { BoardOrientation, BoardThemeId, PieceThemeId } from '@/types';

export interface SettingsState {
  boardTheme: BoardThemeId;
  pieceTheme: PieceThemeId;
  soundEnabled: boolean;
  animationsEnabled: boolean;
  coordinatesVisible: boolean;
  boardOrientation: BoardOrientation;
}

export interface SettingsActions {
  setBoardTheme: (theme: BoardThemeId) => void;
  setPieceTheme: (theme: PieceThemeId) => void;
  setSoundEnabled: (enabled: boolean) => void;
  setAnimationsEnabled: (enabled: boolean) => void;
  setCoordinatesVisible: (visible: boolean) => void;
  setBoardOrientation: (orientation: BoardOrientation) => void;
  flipBoard: () => void;
  resetSettings: () => void;
}

export type SettingsStore = SettingsState & SettingsActions;

export const DEFAULT_SETTINGS: SettingsState = {
  boardTheme: 'classic',
  pieceTheme: 'merida',
  soundEnabled: true,
  animationsEnabled: true,
  coordinatesVisible: true,
  boardOrientation: 'white',
};

export const useSettingsStore = create<SettingsStore>()(
  persist(
    (set) => ({
      ...DEFAULT_SETTINGS,

      setBoardTheme: (boardTheme) => set({ boardTheme }),
      setPieceTheme: (pieceTheme) => set({ pieceTheme }),
      setSoundEnabled: (soundEnabled) => set({ soundEnabled }),
      setAnimationsEnabled: (animationsEnabled) => set({ animationsEnabled }),
      setCoordinatesVisible: (coordinatesVisible) => set({ coordinatesVisible }),
      setBoardOrientation: (boardOrientation) => set({ boardOrientation }),
      flipBoard: () =>
        set((state) => ({
          boardOrientation: state.boardOrientation === 'white' ? 'black' : 'white',
        })),
      resetSettings: () => set({ ...DEFAULT_SETTINGS }),
    }),
    {
      name: 'chess-frontend:settings',
      version: 1,
      partialize: (state): SettingsState => ({
        boardTheme: state.boardTheme,
        pieceTheme: state.pieceTheme,
        soundEnabled: state.soundEnabled,
        animationsEnabled: state.animationsEnabled,
        coordinatesVisible: state.coordinatesVisible,
        boardOrientation: state.boardOrientation,
      }),
    },
  ),
);
