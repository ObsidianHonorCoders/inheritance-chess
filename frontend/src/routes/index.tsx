import { createBrowserRouter } from 'react-router-dom';
import { AppLayout } from '@/components/layout/AppLayout';
import { HomePage } from '@/pages/HomePage';
import { NotFoundPage } from '@/pages/NotFoundPage';
import { SettingsPage } from '@/pages/SettingsPage';
import { UnavailableFeaturePage } from '@/pages/UnavailableFeaturePage';
import { ROUTES } from './paths';
import { RouteErrorBoundary } from './RouteErrorBoundary';

export const router = createBrowserRouter([
  {
    path: ROUTES.home,
    element: <AppLayout />,
    errorElement: <RouteErrorBoundary />,
    children: [
      { index: true, element: <HomePage /> },
      { path: 'play', element: <UnavailableFeaturePage feature="Play" /> },
      { path: 'analysis', element: <UnavailableFeaturePage feature="Analysis" /> },
      { path: 'settings', element: <SettingsPage /> },
      { path: '*', element: <NotFoundPage /> },
    ],
  },
]);

export { ROUTES };
