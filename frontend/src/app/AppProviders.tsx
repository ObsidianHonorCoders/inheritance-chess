import { useEffect, type ReactNode } from 'react';
import { QueryClientProvider } from '@tanstack/react-query';
import { BoardThemeProvider } from '@/features/themes/BoardThemeProvider';
import { PieceThemeProvider } from '@/features/themes/PieceThemeProvider';
import { soundService } from '@/services/soundService';
import { useSettingsStore } from '@/store/settingsStore';
import { queryClient } from './queryClient';

function SoundSettingsSync() {
  const soundEnabled = useSettingsStore((state) => state.soundEnabled);

  useEffect(() => {
    soundService.setEnabled(soundEnabled);
  }, [soundEnabled]);

  useEffect(() => {
    soundService.preload();
  }, []);

  return null;
}

export function AppProviders({ children }: { children: ReactNode }) {
  return (
    <QueryClientProvider client={queryClient}>
      <BoardThemeProvider>
        <PieceThemeProvider>
          <SoundSettingsSync />
          {children}
        </PieceThemeProvider>
      </BoardThemeProvider>
    </QueryClientProvider>
  );
}
