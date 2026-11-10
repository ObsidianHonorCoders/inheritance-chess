import { useMediaQuery } from './useMediaQuery';

export type Breakpoint = 'mobile' | 'tablet' | 'desktop';

/** Mirrors Tailwind's `md` (768px) and `lg` (1024px) breakpoints. */
export function useBreakpoint(): Breakpoint {
  const isDesktop = useMediaQuery('(min-width: 1024px)');
  const isTablet = useMediaQuery('(min-width: 768px)');

  if (isDesktop) return 'desktop';
  return isTablet ? 'tablet' : 'mobile';
}
