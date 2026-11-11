import { Link } from 'react-router-dom';
import { PageTransition } from '@/components/layout/PageTransition';
import { Button } from '@/components/ui/button';
import { ROUTES } from '@/routes/paths';

export function UnavailableFeaturePage({ feature }: { feature: 'Analysis' }) {
  return (
    <PageTransition className="mx-auto flex max-w-xl flex-col items-center gap-4 py-20 text-center">
      <h1 className="text-3xl font-bold">{feature} is unavailable</h1>
      <p className="text-muted-foreground">
        The C++ domain has an engine, but the backend does not expose an analysis endpoint yet.
      </p>
      <Button asChild>
        <Link to={ROUTES.home}>Back to home</Link>
      </Button>
    </PageTransition>
  );
}
