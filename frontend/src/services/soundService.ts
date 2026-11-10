import type { GameStatus, MoveRecord, SoundEvent } from '@/types';

const SOUND_FILES: Record<SoundEvent, string> = {
  move: 'move.wav',
  capture: 'capture.wav',
  castle: 'castle.wav',
  promotion: 'promotion.wav',
  check: 'check.wav',
  checkmate: 'checkmate.wav',
};

const soundUrl = (file: string): string => `${import.meta.env.BASE_URL}sounds/${file}`;

/** Picks the most significant sound for a move (checkmate > check > promotion > castle > capture > move). */
export function getSoundEvent(move: MoveRecord, status: GameStatus): SoundEvent {
  if (status.isCheckmate) return 'checkmate';
  if (status.isCheck) return 'check';
  if (move.promotion) return 'promotion';
  if (move.flags.includes('k') || move.flags.includes('q')) return 'castle';
  if (move.flags.includes('c') || move.flags.includes('e')) return 'capture';
  return 'move';
}

class SoundService {
  private readonly cache = new Map<SoundEvent, HTMLAudioElement>();
  private enabled = true;
  private volume = 0.6;

  setEnabled(enabled: boolean): void {
    this.enabled = enabled;
  }

  setVolume(volume: number): void {
    this.volume = Math.min(1, Math.max(0, volume));
  }

  preload(): void {
    if (typeof Audio === 'undefined') return;
    (Object.keys(SOUND_FILES) as SoundEvent[]).forEach((event) => this.load(event));
  }

  play(event: SoundEvent): void {
    if (!this.enabled || typeof Audio === 'undefined') return;

    // Clone so rapid consecutive sounds can overlap.
    const node = this.load(event).cloneNode(true) as HTMLAudioElement;
    node.volume = this.volume;
    node.play().catch(() => {
      // Autoplay restrictions or missing asset: sounds are best-effort.
    });
  }

  private load(event: SoundEvent): HTMLAudioElement {
    let audio = this.cache.get(event);
    if (!audio) {
      audio = new Audio(soundUrl(SOUND_FILES[event]));
      audio.preload = 'auto';
      this.cache.set(event, audio);
    }
    return audio;
  }
}

export const soundService = new SoundService();
