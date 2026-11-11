import { useReducedMotion, type Transition } from 'framer-motion';
import { useSettingsStore } from '@/store/settingsStore';

/** Animations are on only if the user setting allows it and the OS does not request reduced motion. */
export function useAnimationsEnabled(): boolean {
  const enabled = useSettingsStore((state) => state.animationsEnabled);
  const prefersReducedMotion = useReducedMotion();
  return enabled && !prefersReducedMotion;
}

/** Returns `transition`, or an instant transition when animations are disabled. */
export function useMotionTransition(transition: Transition): Transition {
  const enabled = useAnimationsEnabled();
  return enabled ? transition : { duration: 0 };
}
