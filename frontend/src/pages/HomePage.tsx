import { Link } from 'react-router-dom';
import { Settings, type LucideIcon } from 'lucide-react';
import { motion } from 'framer-motion';
import { PageTransition } from '@/components/layout/PageTransition';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { useMotionTransition } from '@/hooks/useMotionTransition';
import { ROUTES } from '@/routes/paths';

interface HomeAction {
  to: string;
  title: string;
  description: string;
  icon: LucideIcon;
  cta: string;
}

const ACTIONS: readonly HomeAction[] = [
  {
    to: ROUTES.settings,
    title: 'Settings',
    description: 'Customize board and piece themes, sounds and animations.',
    icon: Settings,
    cta: 'Customize',
  },
];

function ActionCard({ action, index }: { action: HomeAction; index: number }) {
  const transition = useMotionTransition({ duration: 0.3, delay: 0.1 + index * 0.08, ease: 'easeOut' });
  const Icon = action.icon;

  return (
    <motion.li initial={{ opacity: 0, y: 12 }} animate={{ opacity: 1, y: 0 }} transition={transition}>
      <Card className="flex h-full flex-col transition-colors hover:border-primary/60">
        <CardHeader>
          <span className="mb-2 inline-flex size-10 items-center justify-center rounded-md bg-primary/15 text-primary">
            <Icon className="size-5" aria-hidden />
          </span>
          <CardTitle>{action.title}</CardTitle>
          <CardDescription>{action.description}</CardDescription>
        </CardHeader>
        <CardContent className="mt-auto">
          <Button asChild variant="secondary" className="w-full">
            <Link to={action.to}>{action.cta}</Link>
          </Button>
        </CardContent>
      </Card>
    </motion.li>
  );
}

export function HomePage() {
  return (
    <PageTransition className="mx-auto max-w-5xl">
      <section className="py-8 text-center md:py-14">
        <h1 className="text-4xl font-bold tracking-tight md:text-5xl">
          Inheritance <span className="text-primary">Chess</span>
        </h1>
        <p className="mx-auto mt-4 max-w-xl text-muted-foreground">
          A chess interface for the Inheritance Chess engine.
        </p>
      </section>

      <section
        className="mx-auto mb-8 max-w-3xl rounded-lg border border-warning/40 bg-warning/5 p-5"
        aria-labelledby="features-unavailable-title"
      >
        <h2 id="features-unavailable-title" className="font-semibold">
          Play and analysis are unavailable
        </h2>
        <p className="mt-2 text-sm text-muted-foreground">
          The backend currently defines game API interfaces but has no concrete game or session
          authentication adapters. It also exposes no engine-analysis endpoint. These screens are
          disabled rather than using local chess rules or mock engine results.
        </p>
      </section>

      <ul className="mx-auto grid max-w-sm gap-4">
        {ACTIONS.map((action, index) => (
          <ActionCard key={action.to} action={action} index={index} />
        ))}
      </ul>
    </PageTransition>
  );
}
