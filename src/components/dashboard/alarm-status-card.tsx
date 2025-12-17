'use client';

import { Volume2, VolumeX, BellRing } from 'lucide-react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { cn } from '@/lib/utils';

interface AlarmStatusCardProps {
  isArmed: boolean;
  soundDetected: boolean;
}

export function AlarmStatusCard({
  isArmed,
  soundDetected,
}: AlarmStatusCardProps) {
  const getStatus = () => {
    if (isArmed && soundDetected) {
      return {
        text: '¡SONIDO DETECTADO!',
        color: 'text-destructive',
        Icon: BellRing,
        pulse: true,
      };
    }
    if (isArmed) {
      return {
        text: 'Escuchando',
        color: 'text-primary',
        Icon: Volume2,
        pulse: false,
      };
    }
    return {
      text: 'Desarmada',
      color: 'text-muted-foreground',
      Icon: VolumeX,
      pulse: false,
    };
  };

  const status = getStatus();

  return (
    <Card className="flex flex-col">
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <status.Icon className="size-5" />
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
