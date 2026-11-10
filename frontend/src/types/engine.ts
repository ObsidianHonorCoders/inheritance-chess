export type ConnectionStatus = 'disconnected' | 'connecting' | 'connected' | 'error';

/** Result of an engine search. `evaluation` is in pawns from White's perspective. */
export interface EngineAnalysis {
  /** Best move in UCI notation (e.g. `e2e4`). */
  bestMove: string | null;
  evaluation: number;
  depth: number;
  nodes: number;
  /** Principal variation in UCI notation. */
  pv: string[];
}

export interface AnalyzeRequest {
  fen: string;
  depth?: number;
  signal?: AbortSignal;
}

/** Contract implemented by both the mock and the future REST/WebSocket client. */
export interface EngineApi {
  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  analyze: (request: AnalyzeRequest) => Promise<EngineAnalysis>;
}
