import { motion } from 'framer-motion';
import { useMotionTransition } from '@/hooks/useMotionTransition';
import type { BoardOrientation } from '@/types';
import { cn } from '@/utils/cn';
import { evaluationToWhitePercent, formatEvaluation, formatEvaluationShort } from '@/utils/format';

export type EvaluationDirection = 'vertical' | 'horizontal';

type Edge = 'top' | 'bottom' | 'left' | 'right';

const LABEL_EDGE_CLASS: Record<Edge, string> = {
  top: 'top-1',
  bottom: 'bottom-1',
  left: 'left-1.5',
  right: 'right-1.5',
};

const OPPOSITE_EDGE: Record<Edge, Edge> = {
  top: 'bottom',
  bottom: 'top',
  left: 'right',
  right: 'left',
};

interface EvaluationBarProps {
  /** Pawns from White's perspective; `null` while unknown. */
  evaluation: number | null;
  orientation?: BoardOrientation;
  direction?: EvaluationDirection;
  className?: string;
}

export function EvaluationBar({
  evaluation,
  orientation = 'white',
  direction = 'vertical',
  className,
}: EvaluationBarProps) {
  const transition = useMotionTransition({ type: 'spring', stiffness: 120, damping: 20 });

  const isVertical = direction === 'vertical';
  const whitePercent = evaluationToWhitePercent(evaluation);
  const whiteLeads = whitePercent >= 50;

  const whiteEdge: Edge = isVertical
    ? orientation === 'white'
      ? 'bottom'
      : 'top'
    : orientation === 'white'
      ? 'left'
      : 'right';
  const labelEdge = whiteLeads ? whiteEdge : OPPOSITE_EDGE[whiteEdge];

  const fillPosition: Record<Edge, string> = {
    bottom: 'inset-x-0 bottom-0',
    top: 'inset-x-0 top-0',
    left: 'inset-y-0 left-0',
    right: 'inset-y-0 right-0',
  };

  const description =
    evaluation === null
      ? 'Evaluation unavailable'
      : `Evaluation ${formatEvaluation(evaluation)}, ${whiteLeads ? 'White' : 'Black'} is better`;

  return (
    <div
      role="img"
      aria-label={description}
      title={description}
      className={cn(
        'relative overflow-hidden rounded-md border bg-zinc-800',
        isVertical ? 'w-7 self-stretch' : 'h-6 w-full',
        className,
      )}
    >
      <motion.div
        className={cn('absolute bg-zinc-100', fillPosition[whiteEdge])}
        initial={false}
        animate={isVertical ? { height: `${whitePercent}%` } : { width: `${whitePercent}%` }}
        transition={transition}
      />
      <span
        className={cn(
          'pointer-events-none absolute text-[10px] font-semibold tabular-nums',
          isVertical ? 'inset-x-0 text-center' : 'inset-y-0 flex items-center',
          LABEL_EDGE_CLASS[labelEdge],
          whiteLeads ? 'text-zinc-900' : 'text-zinc-100',
        )}
      >
        {formatEvaluationShort(evaluation)}
      </span>
    </div>
  );
}
