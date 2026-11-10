import { useMemo, useState, type ChangeEvent } from 'react';
import { Check, Copy, RotateCcw, Upload } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { useCopyToClipboard } from '@/hooks/useCopyToClipboard';
import { useGameStore } from '@/store/gameStore';
import { validateFenString } from '@/utils/fen';

export function PositionEditor() {
  const fen = useGameStore((state) => state.fen);
  const loadFen = useGameStore((state) => state.loadFen);
  const resetGame = useGameStore((state) => state.resetGame);
  const { copied, copy } = useCopyToClipboard();

  // `null` means "follow the current position"; a string is a user edit in progress.
  const [draft, setDraft] = useState<string | null>(null);
  const [loadError, setLoadError] = useState<string | null>(null);

  const value = draft ?? fen;
  const validation = useMemo(() => validateFenString(value), [value]);
  const isDirty = draft !== null;
  const errorText = loadError ?? (isDirty && !validation.ok ? validation.error : null);

  const handleChange = (event: ChangeEvent<HTMLInputElement>) => {
    setDraft(event.target.value);
    setLoadError(null);
  };

  const handleLoad = () => {
    const result = loadFen(value);
    if (result.ok) {
      setDraft(null);
      setLoadError(null);
    } else {
      setLoadError(result.error);
    }
  };

  const handleReset = () => {
    resetGame();
    setDraft(null);
    setLoadError(null);
  };

  return (
    <form
      className="space-y-3"
      onSubmit={(event) => {
        event.preventDefault();
        if (isDirty && validation.ok) handleLoad();
      }}
    >
      <div className="space-y-1.5">
        <Label htmlFor="fen-input">FEN</Label>
        <Input
          id="fen-input"
          value={value}
          onChange={handleChange}
          spellCheck={false}
          autoComplete="off"
          className="font-mono text-xs"
          aria-invalid={errorText !== null}
          aria-describedby="fen-feedback"
        />
        <p
          id="fen-feedback"
          role={errorText ? 'alert' : undefined}
          className={errorText ? 'text-xs text-destructive' : 'text-xs text-muted-foreground'}
        >
          {errorText ?? (isDirty ? 'Valid FEN — press Load to apply.' : 'Showing the current position.')}
        </p>
      </div>

      <div className="flex flex-wrap gap-2">
        <Button type="submit" size="sm" disabled={!isDirty || !validation.ok}>
          <Upload />
          Load
        </Button>
        <Button type="button" size="sm" variant="secondary" onClick={() => void copy(fen)}>
          {copied ? <Check /> : <Copy />}
          {copied ? 'Copied' : 'Copy FEN'}
        </Button>
        <Button type="button" size="sm" variant="secondary" onClick={handleReset}>
          <RotateCcw />
          Reset board
        </Button>
      </div>
    </form>
  );
}
