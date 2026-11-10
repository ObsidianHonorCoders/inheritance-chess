import { Badge } from '@/components/ui/badge';
import { useEngineStore } from '@/store/engineStore';
import { formatEvaluation } from '@/utils/format';
import { PrincipalVariation } from './PrincipalVariation';

// TODO(V3): MultiPV — render one row per engine line and make rows clickable to preview/play them.

export function PVLines() {
  const analyzedFen = useEngineStore((state) => state.analyzedFen);
  const evaluation = useEngineStore((state) => state.evaluation);
  const depth = useEngineStore((state) => state.depth);
  const principalVariation = useEngineStore((state) => state.principalVariation);

  if (principalVariation.length === 0) {
    return <p className="text-sm text-muted-foreground">Waiting for engine output…</p>;
  }

  return (
    <ol className="space-y-2" aria-label="Principal variation lines">
      <li className="flex items-start gap-3 rounded-md border bg-secondary/40 p-2.5">
        <Badge variant="secondary" className="shrink-0 font-mono tabular-nums">
          {formatEvaluation(evaluation)}
        </Badge>
        <div className="min-w-0 flex-1">
          <PrincipalVariation fen={analyzedFen} moves={principalVariation} />
          <p className="mt-1 text-xs text-muted-foreground">Depth {depth ?? '–'}</p>
        </div>
      </li>
    </ol>
  );
}
