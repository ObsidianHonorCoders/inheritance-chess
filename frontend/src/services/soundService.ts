import type { SoundEvent } from '@/types';

const SOUND_FILES: Record<SoundEvent, string> = {
  move: 'move.wav',
  capture: 'capture.wav',
  castle: 'castle.wav',
  promotion: 'promotion.wav',
  check: 'check.wav',
  checkmate: 'checkmate.wav',
};

const soundUrl = (file: string): string => `${import.meta.env.BASE_URL}sounds/${file}`;

/** Picks the most significant sound from server-issued notation and status. */
export function getSoundEvent(move: { uci: string; san: string }, status: string): SoundEvent {
  if (status === 'checkmate') return 'checkmate';
  if (status === 'check') return 'check';
  if (move.uci.length === 5) return 'promotion';
  if (move.san.startsWith('O-O')) return 'castle';
  if (move.san.includes('x')) return 'capture';
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
