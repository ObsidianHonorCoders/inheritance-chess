import { useEngineAnalysis } from '@/features/engine/useEngineAnalysis';
import { useGameKeyboardShortcuts } from './useGameKeyboardShortcuts';

/** Everything a game screen needs on mount: keyboard navigation and live engine analysis. */
export function useGameSession(): void {
  useGameKeyboardShortcuts();
  useEngineAnalysis();
}
