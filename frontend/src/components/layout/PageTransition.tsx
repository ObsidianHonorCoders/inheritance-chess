import type { ReactNode } from 'react';
import { motion } from 'framer-motion';
import { useMotionTransition } from '@/hooks/useMotionTransition';

interface PageTransitionProps {
  children: ReactNode;
  className?: string;
}

export function PageTransition({ children, className }: PageTransitionProps) {
  const transition = useMotionTransition({ duration: 0.25, ease: 'easeOut' });

  return (
    <motion.div
      initial={{ opacity: 0, y: 8 }}
      animate={{ opacity: 1, y: 0 }}
      transition={transition}
      className={className}
    >
      {children}
    </motion.div>
  );
}
