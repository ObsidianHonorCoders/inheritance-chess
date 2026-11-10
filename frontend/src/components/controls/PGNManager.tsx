import { useRef, useState, type ChangeEvent } from 'react';
import { Check, Copy, Download, FileUp, RotateCcw, Upload } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Label } from '@/components/ui/label';
import { Textarea } from '@/components/ui/textarea';
import { useCopyToClipboard } from '@/hooks/useCopyToClipboard';
import { useGameStore } from '@/store/gameStore';
import { downloadTextFile } from '@/utils/download';

export function PGNManager() {
  const pgn = useGameStore((state) => state.pgn);
  const loadPgn = useGameStore((state) => state.loadPgn);
  const resetGame = useGameStore((state) => state.resetGame);
  const { copied, copy } = useCopyToClipboard();
  const fileInputRef = useRef<HTMLInputElement>(null);

  // `null` means "follow the current game"; a string is a user edit in progress.
  const [draft, setDraft] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);

  const value = draft ?? pgn;

  const importText = (text: string) => {
    const result = loadPgn(text);
    if (result.ok) {
      setDraft(null);
      setError(null);
    } else {
      setError(result.error);
    }
  };

  const handleFileChange = async (event: ChangeEvent<HTMLInputElement>) => {
    const input = event.target;
    const file = input.files?.[0];
    if (file) importText(await file.text());
    input.value = '';
  };

  const handleReset = () => {
    resetGame();
    setDraft(null);
    setError(null);
  };

  return (
    <div className="space-y-3">
      <div className="space-y-1.5">
        <Label htmlFor="pgn-input">PGN</Label>
        <Textarea
          id="pgn-input"
          value={value}
          onChange={(event) => {
            setDraft(event.target.value);
            setError(null);
          }}
          rows={6}
          spellCheck={false}
          placeholder="Paste a PGN here, or load a .pgn file"
          className="font-mono text-xs"
          aria-invalid={error !== null}
          aria-describedby="pgn-feedback"
        />
        <p
          id="pgn-feedback"
          role={error ? 'alert' : undefined}
          className={error ? 'text-xs text-destructive' : 'text-xs text-muted-foreground'}
        >
          {error ?? (draft !== null ? 'Press Import to load this PGN.' : 'Showing the current game.')}
        </p>
      </div>

      <div className="flex flex-wrap gap-2">
        <Button type="button" size="sm" disabled={draft === null} onClick={() => importText(value)}>
          <Upload />
          Import
        </Button>
        <Button type="button" size="sm" variant="secondary" onClick={() => fileInputRef.current?.click()}>
          <FileUp />
          Open file
        </Button>
        <Button
          type="button"
          size="sm"
          variant="secondary"
          disabled={!pgn}
          onClick={() => downloadTextFile('game.pgn', pgn, 'application/x-chess-pgn')}
        >
          <Download />
          Export
        </Button>
        <Button type="button" size="sm" variant="secondary" disabled={!pgn} onClick={() => void copy(pgn)}>
          {copied ? <Check /> : <Copy />}
          {copied ? 'Copied' : 'Copy'}
        </Button>
        <Button type="button" size="sm" variant="outline" onClick={handleReset}>
          <RotateCcw />
          Reset game
        </Button>
      </div>

      <input
        ref={fileInputRef}
        type="file"
        accept=".pgn,.txt,text/plain"
        className="sr-only"
        tabIndex={-1}
        aria-hidden
        onChange={(event) => void handleFileChange(event)}
      />
    </div>
  );
}
