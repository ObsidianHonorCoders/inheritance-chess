import { createBrowserRouter } from 'react-router-dom';
import { AppLayout } from '@/components/layout/AppLayout';
import { AnalysisPage } from '@/pages/AnalysisPage';
import { HomePage } from '@/pages/HomePage';
import { NotFoundPage } from '@/pages/NotFoundPage';
import { PlayPage } from '@/pages/PlayPage';
import { SettingsPage } from '@/pages/SettingsPage';
import { ROUTES } from './paths';
import { RouteErrorBoundary } from './RouteErrorBoundary';

export const router = createBrowserRouter([
  {
    path: ROUTES.home,
    element: <AppLayout />,
    errorElement: <RouteErrorBoundary />,
    children: [
      { index: true, element: <HomePage /> },
      { path: 'play', element: <PlayPage /> },
      { path: 'analysis', element: <AnalysisPage /> },
      { path: 'settings', element: <SettingsPage /> },
      { path: '*', element: <NotFoundPage /> },
    ],
  },
]);

export { ROUTES };
