import { Button } from '@/components/ui/button';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle,
} from '@/components/ui/dialog';
import type { Color, PieceCode, PromotionPiece } from '@/types';
import { PieceImage } from './PieceImage';

const OPTIONS: ReadonlyArray<{ piece: PromotionPiece; label: string }> = [
  { piece: 'q', label: 'Queen' },
  { piece: 'r', label: 'Rook' },
  { piece: 'b', label: 'Bishop' },
  { piece: 'n', label: 'Knight' },
];

interface PromotionDialogProps {
  open: boolean;
  color: Color;
  onSelect: (piece: PromotionPiece) => void;
  onCancel: () => void;
}

export function PromotionDialog({ open, color, onSelect, onCancel }: PromotionDialogProps) {
  return (
    <Dialog open={open} onOpenChange={(isOpen) => !isOpen && onCancel()}>
      <DialogContent className="max-w-sm">
        <DialogHeader>
          <DialogTitle>Promote pawn</DialogTitle>
          <DialogDescription>Choose the piece to promote to.</DialogDescription>
        </DialogHeader>
        <div className="grid grid-cols-4 gap-2">
          {OPTIONS.map(({ piece, label }) => (
            <Button
              key={piece}
              type="button"
              variant="outline"
              aria-label={label}
              className="h-auto p-2"
              onClick={() => onSelect(piece)}
            >
              <PieceImage code={`${color}${piece.toUpperCase()}` as PieceCode} className="size-14" />
            </Button>
          ))}
        </div>
      </DialogContent>
    </Dialog>
  );
}
