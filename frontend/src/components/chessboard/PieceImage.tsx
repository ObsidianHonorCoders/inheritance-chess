import { getPieceUrl, usePieceTheme } from '@/features/themes';
import type { PieceCode, PieceThemeId } from '@/types';
import { cn } from '@/utils/cn';

interface PieceImageProps {
  code: PieceCode;
  /** Defaults to the active piece theme. */
  themeId?: PieceThemeId;
  className?: string;
}

export function PieceImage({ code, themeId, className }: PieceImageProps) {
  const { theme } = usePieceTheme();

  return (
    <img
      src={getPieceUrl(themeId ?? theme.id, code)}
      alt=""
      draggable={false}
      className={cn('select-none', className)}
    />
  );
}
