import { useRef, useState, type ReactNode } from 'react';
import { motion } from 'framer-motion';
import { ChevronDown, type LucideIcon } from 'lucide-react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Collapsible, CollapsibleContent, CollapsibleTrigger } from '@/components/ui/collapsible';
import {
  Sheet,
  SheetContent,
  SheetDescription,
  SheetHeader,
  SheetTitle,
} from '@/components/ui/sheet';
import type { Breakpoint } from '@/hooks/useBreakpoint';
import { useMotionTransition } from '@/hooks/useMotionTransition';
import { cn } from '@/utils/cn';

export interface PanelDefinition {
  id: string;
  title: string;
  icon: LucideIcon;
  content: ReactNode;
  /** Tablet only: whether the collapsible starts expanded. */
  defaultOpen?: boolean;
}

interface ResponsivePanelsProps {
  panels: readonly PanelDefinition[];
  breakpoint: Breakpoint;
}

function DesktopPanel({ panel, index }: { panel: PanelDefinition; index: number }) {
  const transition = useMotionTransition({ duration: 0.25, delay: index * 0.05, ease: 'easeOut' });
  const Icon = panel.icon;

  return (
    <motion.section
      initial={{ opacity: 0, x: 16 }}
      animate={{ opacity: 1, x: 0 }}
      transition={transition}
      aria-labelledby={`panel-${panel.id}`}
    >
      <Card>
        <CardHeader className="pb-3">
          <CardTitle id={`panel-${panel.id}`} className="flex items-center gap-2">
            <Icon className="size-4 text-muted-foreground" aria-hidden />
            {panel.title}
          </CardTitle>
        </CardHeader>
        <CardContent>{panel.content}</CardContent>
      </Card>
    </motion.section>
  );
}

function TabletPanel({ panel }: { panel: PanelDefinition }) {
  const Icon = panel.icon;

  return (
    <Collapsible defaultOpen={panel.defaultOpen} asChild>
      <Card>
        <CollapsibleTrigger className="group flex w-full items-center justify-between gap-2 rounded-lg p-4 text-left text-base font-semibold focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring">
          <span className="flex items-center gap-2">
            <Icon className="size-4 text-muted-foreground" aria-hidden />
            {panel.title}
          </span>
          <ChevronDown
            className="size-4 text-muted-foreground transition-transform group-data-[state=open]:rotate-180"
            aria-hidden
          />
        </CollapsibleTrigger>
        <CollapsibleContent>
          <div className="px-4 pb-4">{panel.content}</div>
        </CollapsibleContent>
      </Card>
    </Collapsible>
  );
}

function MobileDrawers({ panels }: { panels: readonly PanelDefinition[] }) {
  const [openId, setOpenId] = useState<string | null>(null);
  const lastShown = useRef<PanelDefinition | null>(null);

  const active = panels.find((panel) => panel.id === openId) ?? null;
  if (active) lastShown.current = active;
  // Keep content mounted while the sheet plays its closing animation.
  const shown = active ?? lastShown.current;

  return (
    <>
      <nav
        aria-label="Panels"
        className="fixed inset-x-0 bottom-0 z-40 border-t bg-background/95 pb-[env(safe-area-inset-bottom)] backdrop-blur"
      >
        <ul className="flex items-stretch justify-around">
          {panels.map((panel) => {
            const Icon = panel.icon;
            return (
              <li key={panel.id} className="flex-1">
                <button
                  type="button"
                  onClick={() => setOpenId(panel.id)}
                  className={cn(
                    'flex w-full flex-col items-center gap-1 py-2 text-xs font-medium text-muted-foreground transition-colors hover:text-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring',
                    openId === panel.id && 'text-primary',
                  )}
                >
                  <Icon className="size-5" aria-hidden />
                  {panel.title}
                </button>
              </li>
            );
          })}
        </ul>
      </nav>

      <Sheet open={openId !== null} onOpenChange={(open) => !open && setOpenId(null)}>
        <SheetContent side="bottom" className="max-h-[80dvh] overflow-y-auto">
          <SheetHeader className="mb-4">
            <SheetTitle>{shown?.title ?? ''}</SheetTitle>
            <SheetDescription className="sr-only">{shown?.title ?? 'Panel'} details</SheetDescription>
          </SheetHeader>
          {shown?.content}
        </SheetContent>
      </Sheet>
    </>
  );
}

/** Desktop: stacked cards. Tablet: collapsible cards. Mobile: bottom navigation with drawers. */
export function ResponsivePanels({ panels, breakpoint }: ResponsivePanelsProps) {
  if (breakpoint === 'desktop') {
    return (
      <div className="flex flex-col gap-4">
        {panels.map((panel, index) => (
          <DesktopPanel key={panel.id} panel={panel} index={index} />
        ))}
      </div>
    );
  }

  if (breakpoint === 'tablet') {
    return (
      <div className="grid items-start gap-4 md:grid-cols-2">
        {panels.map((panel) => (
          <TabletPanel key={panel.id} panel={panel} />
        ))}
      </div>
    );
  }

  return <MobileDrawers panels={panels} />;
}
