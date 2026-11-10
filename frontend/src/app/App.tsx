import { RouterProvider } from 'react-router-dom';
import { router } from '@/routes';
import { AppProviders } from './AppProviders';

export function App() {
  return (
    <AppProviders>
      <RouterProvider router={router} />
    </AppProviders>
  );
}
