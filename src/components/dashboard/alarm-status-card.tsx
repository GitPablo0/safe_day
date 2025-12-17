'use client';

import { Volume2, VolumeX } from 'lucide-react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { cn } from '@/lib/utils';

interface AlarmStatusCardProps {
  isArmed: boolean;
  soundDetected: boolean;
}

function CryingBabyIcon(props: React.SVGProps<SVGSVGElement>) {
  return (
    <svg
      {...props}
      xmlns="http://www.w3.org/2000/svg"
      width="24"
      height="24"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M12 2a10 10 0 1 0 10 10A10 10 0 0 0 12 2" />
      <path d="M8 14c.5-1 2-2 4-2s3.5 1 4 2" />
      <path d="M9 9h.01" />
      <path d="M15 9h.01" />
      <path d="M17.5 16.5a2.5 2.5 0 0 1-5 0" />
      <path d="M8.5 18H6.5a2 2 0 0 1-2-2v-1.5" />
      <path d="M17.5 14.5V16a2 2 0 0 1-2 2h-2" />
    </svg>
  );
}


export function AlarmStatusCard({
  isArmed,
  soundDetected,
}: AlarmStatusCardProps) {
  const getStatus = () => {
    if (isArmed && soundDetected) {
      return {
        text: '¡Bebé llorando!',
        color: 'text-primary',
        Icon: CryingBabyIcon,
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
            status.pulse && 'animate-pulse bg-primary/20'
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
