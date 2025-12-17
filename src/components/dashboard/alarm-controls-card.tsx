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
import { Settings } from 'lucide-react';
import { updateAlarmState } from '@/firebase/firestore/mutations';
import { useFirestore, useUser } from '@/firebase';
import { cn } from '@/lib/utils';

interface AlarmControlsCardProps {
  isArmed: boolean;
  isDeviceConnected: boolean; 
}

export function AlarmControlsCard({
  isArmed,
  isDeviceConnected,
}: AlarmControlsCardProps) {
  const firestore = useFirestore();
  const { user } = useUser();

  const handleArmChange = (armed: boolean) => {
    if (!firestore || !user) return;
    updateAlarmState(firestore, user.uid, { isArmed: armed, soundDetected: false });
  };

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
      </CardContent>
    </Card>
  );
}
