import { useEffect } from 'react';
import { useGameStore } from '@/store/gameStore';

function isEditableTarget(target: EventTarget | null): boolean {
  return (
    target instanceof HTMLElement &&
    (target.isContentEditable || ['INPUT', 'TEXTAREA', 'SELECT'].includes(target.tagName))
  );
}

/** Arrow keys step through moves; Home/End jump to the first/last position. */
export function useGameKeyboardShortcuts(): void {
  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      if (event.defaultPrevented || event.altKey || event.ctrlKey || event.metaKey) return;
      if (isEditableTarget(event.target)) return;

      const game = useGameStore.getState();
      switch (event.key) {
        case 'ArrowLeft':
          game.goToPrevious();
          break;
        case 'ArrowRight':
          game.goToNext();
          break;
        case 'Home':
          game.goToStart();
          break;
        case 'End':
          game.goToEnd();
          break;
        default:
          return;
      }
      event.preventDefault();
    };

    window.addEventListener('keydown', onKeyDown);
    return () => window.removeEventListener('keydown', onKeyDown);
  }, []);
}
