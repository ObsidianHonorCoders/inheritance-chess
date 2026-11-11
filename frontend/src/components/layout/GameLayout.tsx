import type { ReactNode } from 'react';
import { useBreakpoint } from '@/hooks/useBreakpoint';
import type { EvaluationDirection } from '@/components/evaluation/EvaluationBar';
import { ResponsivePanels, type PanelDefinition } from './ResponsivePanels';

interface GameLayoutProps {
  board: ReactNode;
  /** Rendered next to the board (vertical) or above it on mobile (horizontal). */
  evaluationBar: (direction: EvaluationDirection) => ReactNode;
  header?: ReactNode;
  toolbar?: ReactNode;
  panels: readonly PanelDefinition[];
}

/**
 * Desktop: board + side panels. Tablet: board + collapsible panels below.
 * Mobile: board + bottom drawers.
 */
export function GameLayout({ board, evaluationBar, header, toolbar, panels }: GameLayoutProps) {
  const breakpoint = useBreakpoint();
  const isMobile = breakpoint === 'mobile';

  const stage = (
    <div className="mx-auto w-full" style={{ maxWidth: 'min(100%, calc(100dvh - 11rem))' }}>
      {header}
      {isMobile ? (
        <div className="space-y-2">
          {evaluationBar('horizontal')}
          {board}
        </div>
      ) : (
        <div className="flex items-stretch gap-3">
          {evaluationBar('vertical')}
          <div className="min-w-0 flex-1">{board}</div>
        </div>
      )}
      <div className="mt-3">{toolbar}</div>
    </div>
  );

  if (breakpoint === 'desktop') {
    return (
      <div className="mx-auto grid w-full max-w-[96rem] items-start gap-6 lg:grid-cols-[minmax(0,1fr)_24rem]">
        {stage}
        <ResponsivePanels panels={panels} breakpoint={breakpoint} />
      </div>
    );
  }

  return (
    <div className={isMobile ? 'space-y-4 pb-20' : 'mx-auto w-full max-w-4xl space-y-4'}>
      {stage}
      <ResponsivePanels panels={panels} breakpoint={breakpoint} />
    </div>
  );
}
