import { useMemo } from 'react';
import { Badge, type BadgeProps } from '@/components/ui/badge';
import { Label } from '@/components/ui/label';
import { Switch } from '@/components/ui/switch';
import { useEngineStore } from '@/store/engineStore';
import { uciToSanLine } from '@/utils/chess';
import { formatEvaluation, formatNodes } from '@/utils/format';
import { PrincipalVariation } from './PrincipalVariation';

function Stat({ label, value }: { label: string; value: string }) {
  return (
    <div className="rounded-md border bg-secondary/40 px-3 py-2">
      <dt className="text-xs text-muted-foreground">{label}</dt>
      <dd className="font-mono text-base font-semibold tabular-nums">{value}</dd>
    </div>
  );
}

export function EnginePanel() {
  const connectionStatus = useEngineStore((state) => state.connectionStatus);
  const isAnalyzing = useEngineStore((state) => state.isAnalyzing);
  const analysisEnabled = useEngineStore((state) => state.analysisEnabled);
  const setAnalysisEnabled = useEngineStore((state) => state.setAnalysisEnabled);
  const analyzedFen = useEngineStore((state) => state.analyzedFen);
  const bestMove = useEngineStore((state) => state.bestMove);
  const evaluation = useEngineStore((state) => state.evaluation);
  const depth = useEngineStore((state) => state.depth);
  const nodes = useEngineStore((state) => state.nodes);
  const principalVariation = useEngineStore((state) => state.principalVariation);

  const bestMoveSan = useMemo(() => {
    if (!bestMove || !analyzedFen) return null;
    return uciToSanLine(analyzedFen, [bestMove]).at(0)?.san ?? bestMove;
  }, [bestMove, analyzedFen]);

  let statusLabel: string = connectionStatus;
  let statusVariant: BadgeProps['variant'] = 'secondary';
  if (connectionStatus === 'connected') {
    statusLabel = isAnalyzing && analysisEnabled ? 'analyzing' : 'connected';
    statusVariant = isAnalyzing && analysisEnabled ? 'default' : 'success';
  } else if (connectionStatus === 'connecting') {
    statusVariant = 'warning';
  } else if (connectionStatus === 'error') {
    statusVariant = 'destructive';
  }

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between gap-3">
        <Badge variant={statusVariant} className="capitalize" role="status">
          {statusLabel}
        </Badge>
        <div className="flex items-center gap-2">
          <Label htmlFor="engine-analysis-toggle" className="text-muted-foreground">
            Analysis
          </Label>
          <Switch
            id="engine-analysis-toggle"
            checked={analysisEnabled}
            onCheckedChange={setAnalysisEnabled}
          />
        </div>
      </div>

      <dl className="grid grid-cols-2 gap-2" aria-live="polite">
        <Stat label="Evaluation" value={formatEvaluation(evaluation)} />
        <Stat label="Best move" value={bestMoveSan ?? '–'} />
        <Stat label="Depth" value={depth === null ? '–' : String(depth)} />
        <Stat label="Nodes" value={formatNodes(nodes)} />
      </dl>

      <div className="space-y-1.5">
        <h4 className="text-xs font-medium uppercase tracking-wide text-muted-foreground">
          Principal variation
        </h4>
        <PrincipalVariation fen={analyzedFen} moves={principalVariation} />
      </div>
    </div>
  );
}
