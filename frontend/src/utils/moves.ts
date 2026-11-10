import type { MoveRecord } from '@/types';

export interface MoveRow {
  moveNumber: number;
  white?: MoveRecord;
  black?: MoveRecord;
}

/** Groups plies into numbered rows; handles positions where Black moves first. */
export function buildMoveRows(history: readonly MoveRecord[]): MoveRow[] {
  const rows: MoveRow[] = [];
  for (const record of history) {
    const last = rows[rows.length - 1];
    if (record.color === 'w') {
      rows.push({ moveNumber: record.moveNumber, white: record });
    } else if (last && last.moveNumber === record.moveNumber && last.white && !last.black) {
      last.black = record;
    } else {
      rows.push({ moveNumber: record.moveNumber, black: record });
    }
  }
  return rows;
}
