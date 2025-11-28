'use client';

import { Shield, ShieldAlert, ShieldCheck } from 'lucide-react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { cn } from '@/lib/utils';
import { ShieldIcon } from '../icons/shield-icon';

interface AlarmStatusCardProps {
  isArmed: boolean;
  presenceDetected: boolean;
}

export function AlarmStatusCard({
  isArmed,
  presenceDetected,
}: AlarmStatusCardProps) {
  const getStatus = () => {
    if (isArmed && presenceDetected) {
      return {
        text: '¡ACTIVADA!',
        color: 'text-destructive',
        Icon: ShieldAlert,
        pulse: true,
      };
    }
    if (isArmed) {
      return {
        text: 'Armada',
        color: 'text-primary',
        Icon: ShieldCheck,
        pulse: false,
      };
    }
    return {
      text: 'Desarmada',
      color: 'text-muted-foreground',
      Icon: Shield,
      pulse: false,
    };
  };

  const status = getStatus();

  return (
    <Card className="flex flex-col">
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <ShieldIcon className="size-5" />
          <span>Estado del Sistema</span>
        </CardTitle>
      </CardHeader>
      <CardContent className="flex flex-1 flex-col items-center justify-center gap-4">
        <div
          className={cn(
            'relative flex h-32 w-32 items-center justify-center rounded-full',
            status.pulse && 'animate-pulse bg-destructive/20'
          )}
        >
          <status.Icon className={cn('h-16 w-16', status.color)} />
        </div>
        <p className={cn('text-2xl font-bold', status.color)}>
          {status.text}
        </p>
      </CardContent>
    </Card>
  );
}
