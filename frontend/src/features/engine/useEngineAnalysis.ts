import { useEffect } from 'react';
import { keepPreviousData, useQuery } from '@tanstack/react-query';
import { engineApi } from '@/services/engineApi';
import { useEngineStore } from '@/store/engineStore';
import { useGameStore } from '@/store/gameStore';

export const engineQueryKeys = {
  analysis: (fen: string) => ['engine', 'analysis', fen] as const,
};

/** Fetches analysis for the displayed position and mirrors the result into the engine store. */
export function useEngineAnalysis(): void {
  const fen = useGameStore((state) => state.fen);
  const analysisEnabled = useEngineStore((state) => state.analysisEnabled);
  const connectionStatus = useEngineStore((state) => state.connectionStatus);
  const setAnalysis = useEngineStore((state) => state.setAnalysis);
  const setAnalyzing = useEngineStore((state) => state.setAnalyzing);

  const query = useQuery({
    queryKey: engineQueryKeys.analysis(fen),
    queryFn: ({ signal }) => engineApi.analyze({ fen, signal }),
    enabled: analysisEnabled && connectionStatus === 'connected',
    staleTime: Infinity,
    placeholderData: keepPreviousData,
  });

  const { data, isPlaceholderData, isFetching } = query;

  useEffect(() => {
    if (data && !isPlaceholderData) setAnalysis(fen, data);
  }, [data, isPlaceholderData, fen, setAnalysis]);

  useEffect(() => {
    setAnalyzing(isFetching);
  }, [isFetching, setAnalyzing]);
}
