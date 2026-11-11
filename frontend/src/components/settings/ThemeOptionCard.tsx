import type { ReactNode } from 'react';
import { motion } from 'framer-motion';
import { Check } from 'lucide-react';
import { RadioGroupItem } from '@/components/ui/radio-group';
import { useMotionTransition } from '@/hooks/useMotionTransition';

interface ThemeOptionCardProps {
  value: string;
  label: string;
  selected: boolean;
  /** Shared `layoutId` so the selection ring slides between options. */
  indicatorId: string;
  children: ReactNode;
}

export function ThemeOptionCard({ value, label, selected, indicatorId, children }: ThemeOptionCardProps) {
  const transition = useMotionTransition({ type: 'spring', stiffness: 420, damping: 34 });

  return (
    <RadioGroupItem
      value={value}
      className="relative flex flex-col gap-2 rounded-lg border bg-card p-3 text-left transition-colors hover:bg-accent"
    >
      {selected && (
        <motion.span
          layoutId={indicatorId}
          transition={transition}
          className="pointer-events-none absolute inset-0 rounded-lg ring-2 ring-primary"
        />
      )}
      {children}
      <span className="flex items-center justify-between text-sm font-medium">
        {label}
        {selected && <Check className="size-4 text-primary" aria-hidden />}
      </span>
    </RadioGroupItem>
  );
}
