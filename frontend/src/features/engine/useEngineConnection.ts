import { useEffect } from 'react';
import { engineApi } from '@/services/engineApi';
import { useEngineStore } from '@/store/engineStore';

/** Owns the engine connection lifecycle for the whole app (mounted once in the layout). */
export function useEngineConnection(): void {
  const setConnectionStatus = useEngineStore((state) => state.setConnectionStatus);

  useEffect(() => {
    let cancelled = false;
    setConnectionStatus('connecting');

    engineApi
      .connect()
      .then(() => {
        if (!cancelled) setConnectionStatus('connected');
      })
      .catch(() => {
        if (!cancelled) setConnectionStatus('error');
      });

    return () => {
      cancelled = true;
      void engineApi.disconnect();
      setConnectionStatus('disconnected');
    };
  }, [setConnectionStatus]);
}
