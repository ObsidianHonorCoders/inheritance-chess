import { useGameKeyboardShortcuts } from '@/features/game/useGameKeyboardShortcuts';
import { useEngineAnalysis } from '@/features/engine/useEngineAnalysis';

// TODO(V3): MultiPV lines, candidate moves and engine arrows are orchestrated here.

/** Setup for the analysis screen: keyboard navigation and live engine analysis. */
export function useAnalysisSession(): void {
  useGameKeyboardShortcuts();
  useEngineAnalysis();
}
