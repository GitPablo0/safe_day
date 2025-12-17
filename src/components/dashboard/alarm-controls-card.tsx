'use client';

import { useState, useEffect } from 'react';
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from '@/components/ui/card';
import { Label } from '@/components/ui/label';
import { Switch } from '@/components/ui/switch';
import { Slider } from '@/components/ui/slider';
import { Settings, Timer } from 'lucide-react';
import { updateAlarmState } from '@/firebase/firestore/mutations';
import { useFirestore, useUser } from '@/firebase';
import { cn } from '@/lib/utils';

interface AlarmControlsCardProps {
  isArmed: boolean;
  isDeviceConnected: boolean;
  initialDuration: number;
}

export function AlarmControlsCard({
  isArmed,
  isDeviceConnected,
  initialDuration,
}: AlarmControlsCardProps) {
  const firestore = useFirestore();
  const { user } = useUser();
  const [duration, setDuration] = useState(initialDuration);

  useEffect(() => {
    setDuration(initialDuration);
  }, [initialDuration]);

  const handleArmChange = (armed: boolean) => {
    if (!firestore || !user) return;
    updateAlarmState(firestore, user.uid, { 
      isArmed: armed, 
      soundDetected: false,
      alarmDuration: duration
    });
  };

  const handleDurationChange = (value: number[]) => {
    setDuration(value[0]);
  };
  
  const handleDurationCommit = (value: number[]) => {
    if (!firestore || !user) return;
    updateAlarmState(firestore, user.uid, { alarmDuration: value[0] });
  }

  return (
    <Card className={cn(!isDeviceConnected && 'border-dashed bg-muted/20')}>
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <Settings className="size-5" />
          <span>Controles de Alarma</span>
        </CardTitle>
        <CardDescription>
          Ajusta la configuración de tu sistema de alarma por sonido.
          {!isDeviceConnected && (
            <span className="mt-1 block text-xs text-primary">
              Usa un control para inicializar el estado del dispositivo.
            </span>
          )}
        </CardDescription>
      </CardHeader>
      <CardContent className="space-y-8 pt-6">
        <div className="flex items-center justify-between space-x-2">
          <Label htmlFor="arm-switch" className="flex flex-col space-y-1">
            <span className="font-medium">Armar Alarma</span>
            <span className="font-normal text-muted-foreground">
              Activa o desactiva el monitoreo de sonido.
            </span>
          </Label>
          <Switch
            id="arm-switch"
            checked={isArmed}
            onCheckedChange={handleArmChange}
            aria-label="Armar alarma"
          />
        </div>

        <div className="space-y-4">
          <div className="flex items-center justify-between">
            <Label htmlFor="duration-slider" className="flex flex-col space-y-1">
              <span className="font-medium flex items-center gap-2"><Timer className="size-4"/>Temporizador</span>
              <span className="font-normal text-muted-foreground">
                Tiempo que la alarma permanecerá activa.
              </span>
            </Label>
            <span className="text-sm font-bold text-primary">{duration} min</span>
          </div>
          <Slider
            id="duration-slider"
            min={2}
            max={20}
            step={1}
            value={[duration]}
            onValueChange={handleDurationChange}
            onValueCommit={handleDurationCommit}
            disabled={isArmed}
          />
        </div>
      </CardContent>
    </Card>
  );
}
