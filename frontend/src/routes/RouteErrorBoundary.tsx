import { isRouteErrorResponse, Link, useRouteError } from 'react-router-dom';
import { Button } from '@/components/ui/button';
import { ROUTES } from './paths';

export function RouteErrorBoundary() {
  const error = useRouteError();
  const message = isRouteErrorResponse(error)
    ? `${error.status} ${error.statusText}`
    : error instanceof Error
      ? error.message
      : 'Unexpected error';

  return (
    <div className="mx-auto flex min-h-screen max-w-md flex-col items-center justify-center gap-4 px-4 text-center">
      <h1 className="text-2xl font-semibold">Something went wrong</h1>
      <p className="text-sm text-muted-foreground">{message}</p>
      <Button asChild>
        <Link to={ROUTES.home}>Back to home</Link>
      </Button>
    </div>
  );
}
