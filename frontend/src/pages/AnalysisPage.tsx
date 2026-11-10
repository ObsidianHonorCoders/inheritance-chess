import { Cpu, FileText, GitBranch, ListOrdered, PencilLine } from 'lucide-react';
import { EvaluationBar } from '@/components/evaluation/EvaluationBar';
import { BoardControls } from '@/components/controls/BoardControls';
import { GameStatusBanner } from '@/components/controls/GameStatusBanner';
import { PGNManager } from '@/components/controls/PGNManager';
import { PositionEditor } from '@/components/controls/PositionEditor';
import { EnginePanel } from '@/components/engine/EnginePanel';
import { PVLines } from '@/components/engine/PVLines';
import { GameLayout } from '@/components/layout/GameLayout';
import { PageTransition } from '@/components/layout/PageTransition';
import type { PanelDefinition } from '@/components/layout/ResponsivePanels';
import { MoveListPanel } from '@/components/movelist/MoveListPanel';
import { useAnalysisSession } from '@/features/analysis/useAnalysisSession';
import { GameBoard } from '@/features/game/GameBoard';
import { useEngineStore } from '@/store/engineStore';
import { useSettingsStore } from '@/store/settingsStore';

// TODO(V3): opening explorer panel, engine arrows on the board, candidate moves.

const PANELS: readonly PanelDefinition[] = [
  { id: 'engine', title: 'Engine', icon: Cpu, content: <EnginePanel />, defaultOpen: true },
  { id: 'lines', title: 'PV Lines', icon: GitBranch, content: <PVLines />, defaultOpen: true },
  { id: 'position', title: 'Position', icon: PencilLine, content: <PositionEditor /> },
  { id: 'moves', title: 'Moves', icon: ListOrdered, content: <MoveListPanel /> },
  { id: 'pgn', title: 'PGN', icon: FileText, content: <PGNManager /> },
];

export function AnalysisPage() {
  useAnalysisSession();
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
