'use client';

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
import { Settings } from 'lucide-react';
import { updateAlarmState } from '@/firebase/firestore/mutations';
import type { Firestore } from 'firebase/firestore';
import type { Database } from 'firebase/database';
import { useAuth, useRtdb } from '@/firebase';

interface AlarmControlsCardProps {
  isArmed: boolean;
  alarmDuration: number;
  firestore: Firestore;
  isDeviceConnected: boolean;
}

export function AlarmControlsCard({
  isArmed,
  alarmDuration,
  firestore,
  isDeviceConnected,
}: AlarmControlsCardProps) {
  const rtdb = useRtdb();

  const handleArmChange = (armed: boolean) => {
    if (!rtdb) return;
    updateAlarmState(firestore, rtdb, { isArmed: armed, presenceDetected: false }); // Always reset presence on arm/disarm
  };

  const handleDurationChange = (value: number[]) => {
    if (!rtdb) return;
    updateAlarmState(firestore, rtdb, { alarmDuration: value[0] });
  };

  return (
    <Card className={!isDeviceConnected ? 'bg-muted/30' : ''}>
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <Settings className="size-5" />
          <span>Controles</span>
        </CardTitle>
        <CardDescription>
          Ajusta la configuración de tu sistema de alarma.
        </CardDescription>
      </CardHeader>
      <CardContent className="space-y-8 pt-2">
        <div className="flex items-center justify-between space-x-2">
          <Label htmlFor="arm-switch" className="flex flex-col space-y-1">
            <span className="font-medium">Armar Alarma</span>
            <span className="font-normal text-muted-foreground">
              Activa o desactiva el monitoreo de presencia.
            </span>
          </Label>
          <Switch
            id="arm-switch"
            checked={isArmed}
            onCheckedChange={handleArmChange}
            aria-label="Armar alarma"
            disabled={!isDeviceConnected}
          />
        </div>

        <div className="space-y-4">
          <Label htmlFor="duration-slider" className="flex flex-col space-y-1">
            <span className="font-medium">Duración de la Sirena</span>
            <span className="font-normal text-muted-foreground">
              Tiempo que sonará el buzzer al activarse.
            </span>
          </Label>
          <div className="flex items-center gap-4">
            <Slider
              id="duration-slider"
              min={5}
              max={60}
              step={5}
              value={[alarmDuration]}
              onValueChange={handleDurationChange}
              disabled={isArmed || !isDeviceConnected}
              aria-label="Duración de la sirena"
            />
            <span className="w-12 text-center font-mono text-lg">
              {alarmDuration}s
            </span>
          </div>
        </div>
      </CardContent>
    </Card>
  );
}
