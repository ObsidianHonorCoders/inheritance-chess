import { Link } from 'react-router-dom';
import { PageTransition } from '@/components/layout/PageTransition';
import { Button } from '@/components/ui/button';
import { ROUTES } from '@/routes/paths';

export function UnavailableFeaturePage({ feature }: { feature: 'Play' | 'Analysis' }) {
  const reason =
    feature === 'Play'
      ? 'The backend game API has no concrete game-application or session-authentication implementation yet.'
      : 'The domain has a C++ chess engine, but the backend exposes no analysis endpoint and the frontend engine is only a mock.';

  return (
    <PageTransition className="mx-auto flex max-w-xl flex-col items-center gap-4 py-20 text-center">
      <h1 className="text-3xl font-bold">{feature} is unavailable</h1>
      <p className="text-muted-foreground">{reason}</p>
      <Button asChild>
        <Link to={ROUTES.home}>Back to home</Link>
      </Button>
    </PageTransition>
  );
}
