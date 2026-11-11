import { NavLink, Link } from 'react-router-dom';
import { Crown, Settings, Swords, type LucideIcon } from 'lucide-react';
import logoUrl from '@/assets/icons/logo.svg';
import { ROUTES } from '@/routes/paths';
import { cn } from '@/utils/cn';

interface NavItem {
  to: string;
  label: string;
  icon: LucideIcon;
  end?: boolean;
}

const NAV_ITEMS: readonly NavItem[] = [
  { to: ROUTES.home, label: 'Home', icon: Crown, end: true },
  { to: ROUTES.play, label: 'Play', icon: Swords },
  { to: ROUTES.settings, label: 'Settings', icon: Settings },
];

export function Header() {
  return (
    <header className="sticky top-0 z-30 border-b bg-background/80 backdrop-blur">
      <div className="mx-auto flex h-14 max-w-[96rem] items-center justify-between gap-4 px-4">
        <Link to={ROUTES.home} className="flex items-center gap-2 font-semibold tracking-tight">
          <img src={logoUrl} alt="" className="size-7" />
          <span className="hidden sm:inline">Inheritance Chess</span>
        </Link>

        <nav aria-label="Main">
          <ul className="flex items-center gap-1">
            {NAV_ITEMS.map(({ to, label, icon: Icon, end }) => (
              <li key={to}>
                <NavLink
                  to={to}
                  end={end}
                  aria-label={label}
                  className={({ isActive }) =>
                    cn(
                      'flex items-center gap-2 rounded-md px-3 py-2 text-sm font-medium text-muted-foreground transition-colors hover:bg-accent hover:text-accent-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring',
                      isActive && 'bg-accent text-foreground',
                    )
                  }
                >
                  <Icon className="size-4" aria-hidden />
                  <span className="hidden md:inline">{label}</span>
                </NavLink>
              </li>
            ))}
          </ul>
        </nav>
      </div>
    </header>
  );
}
