import { create } from 'zustand';
import type { ConnectionStatus, EngineAnalysis } from '@/types';

// TODO(V3): MultiPV — replace the single `principalVariation` with `lines: EngineLine[]`.
// TODO(V3): expose candidate moves / arrows data derived from the analysis.
// TODO(V4): engine-vs-engine and tournament state (multiple engine slots).

export interface EngineState {
  connectionStatus: ConnectionStatus;
  analysisEnabled: boolean;
  isAnalyzing: boolean;
  /** FEN the stored analysis belongs to (needed to render UCI lines as SAN). */
  analyzedFen: string | null;
  bestMove: string | null;
  /** Pawns, White's perspective. */
  evaluation: number | null;
  depth: number | null;
  nodes: number | null;
  principalVariation: string[];
}

export interface EngineActions {
  setConnectionStatus: (status: ConnectionStatus) => void;
  setAnalysisEnabled: (enabled: boolean) => void;
  setAnalyzing: (analyzing: boolean) => void;
  setAnalysis: (fen: string, analysis: EngineAnalysis) => void;
  resetAnalysis: () => void;
}

export type EngineStore = EngineState & EngineActions;

const INITIAL_ANALYSIS: Pick<
  EngineState,
  'analyzedFen' | 'bestMove' | 'evaluation' | 'depth' | 'nodes' | 'principalVariation'
> = {
  analyzedFen: null,
  bestMove: null,
  evaluation: null,
  depth: null,
  nodes: null,
  principalVariation: [],
};

export const useEngineStore = create<EngineStore>()((set) => ({
  connectionStatus: 'disconnected',
  analysisEnabled: true,
  isAnalyzing: false,
  ...INITIAL_ANALYSIS,

  setConnectionStatus: (connectionStatus) => set({ connectionStatus }),
  setAnalysisEnabled: (analysisEnabled) => set({ analysisEnabled }),
  setAnalyzing: (isAnalyzing) => set({ isAnalyzing }),
  setAnalysis: (fen, analysis) =>
    set({
      analyzedFen: fen,
      bestMove: analysis.bestMove,
      evaluation: analysis.evaluation,
      depth: analysis.depth,
      nodes: analysis.nodes,
      principalVariation: analysis.pv,
    }),
  resetAnalysis: () => set({ ...INITIAL_ANALYSIS }),
}));
