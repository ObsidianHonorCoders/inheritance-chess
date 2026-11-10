import { useCallback } from 'react';
import { getSoundEvent, soundService } from '@/services/soundService';
import { useGameStore } from '@/store/gameStore';
import type { MoveInput, MoveRecord } from '@/types';

/** Game mutations that have side effects beyond the store (sounds today; clocks/network later). */
export function useGameActions() {
  const makeMove = useGameStore((state) => state.makeMove);

  const playMove = useCallback(
    (input: MoveInput): MoveRecord | null => {
      const record = makeMove(input);
      if (record) soundService.play(getSoundEvent(record, useGameStore.getState().status));
      return record;
    },
    [makeMove],
  );

  return { playMove };
}
