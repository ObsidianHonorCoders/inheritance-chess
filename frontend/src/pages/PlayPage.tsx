import { Cpu, FileText, ListOrdered } from 'lucide-react';
import { EvaluationBar } from '@/components/evaluation/EvaluationBar';
import { BoardControls } from '@/components/controls/BoardControls';
import { GameStatusBanner } from '@/components/controls/GameStatusBanner';
import { PGNManager } from '@/components/controls/PGNManager';
import { EnginePanel } from '@/components/engine/EnginePanel';
import { GameLayout } from '@/components/layout/GameLayout';
import { PageTransition } from '@/components/layout/PageTransition';
import type { PanelDefinition } from '@/components/layout/ResponsivePanels';
import { MoveListPanel } from '@/components/movelist/MoveListPanel';
import { GameBoard } from '@/features/game/GameBoard';
import { useGameSession } from '@/features/game/useGameSession';
import { useEngineStore } from '@/store/engineStore';
import { useSettingsStore } from '@/store/settingsStore';

// TODO(V2): clocks and captured-piece strips above/below the board.
// TODO(V4): engine opponent, engine vs engine, online multiplayer, cloud game storage.

const PANELS: readonly PanelDefinition[] = [
  { id: 'moves', title: 'Moves', icon: ListOrdered, content: <MoveListPanel />, defaultOpen: true },
  { id: 'engine', title: 'Engine', icon: Cpu, content: <EnginePanel />, defaultOpen: true },
  { id: 'pgn', title: 'PGN', icon: FileText, content: <PGNManager /> },
];

export function PlayPage() {
  useGameSession();
  const evaluation = useEngineStore((state) => state.evaluation);
  const orientation = useSettingsStore((state) => state.boardOrientation);

  return (
    <PageTransition>
      <GameLayout
        header={<GameStatusBanner />}
        board={<GameBoard />}
        evaluationBar={(direction) => (
          <EvaluationBar evaluation={evaluation} orientation={orientation} direction={direction} />
        )}
        toolbar={<BoardControls />}
        panels={PANELS}
      />
    </PageTransition>
  );
}
