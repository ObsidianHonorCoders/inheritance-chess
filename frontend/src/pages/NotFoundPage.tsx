import { Link } from 'react-router-dom';
import { PageTransition } from '@/components/layout/PageTransition';
import { Button } from '@/components/ui/button';
import { ROUTES } from '@/routes/paths';

export function NotFoundPage() {
  return (
    <PageTransition className="mx-auto flex max-w-md flex-col items-center gap-4 py-20 text-center">
      <h1 className="text-3xl font-bold">Page not found</h1>
      <p className="text-muted-foreground">The page you are looking for does not exist.</p>
      <Button asChild>
        <Link to={ROUTES.home}>Back to home</Link>
      </Button>
    </PageTransition>
  );
}
