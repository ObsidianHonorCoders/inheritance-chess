import type { ReactNode } from 'react';
import { Label } from '@/components/ui/label';
import { Switch } from '@/components/ui/switch';

interface ToggleSettingProps {
  id: string;
  label: string;
  description: string;
  icon?: ReactNode;
  checked: boolean;
  onCheckedChange: (checked: boolean) => void;
}

export function ToggleSetting({ id, label, description, icon, checked, onCheckedChange }: ToggleSettingProps) {
  return (
    <div className="flex items-center justify-between gap-4">
      <div className="flex items-start gap-3">
        {icon && <span className="mt-0.5 text-muted-foreground">{icon}</span>}
        <div className="space-y-1">
          <Label htmlFor={id}>{label}</Label>
          <p className="text-sm text-muted-foreground">{description}</p>
        </div>
      </div>
      <Switch id={id} checked={checked} onCheckedChange={onCheckedChange} />
    </div>
  );
}
