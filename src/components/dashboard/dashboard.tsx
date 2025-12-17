'use client';

import { useEffect, useMemo } from 'react';
import { useRouter } from 'next/navigation';
import { useUser, useFirestore } from '@/firebase';
import { doc, Timestamp } from 'firebase/firestore';
import { useDoc } from '@/firebase/firestore/use-doc';

import { AlarmStatusCard } from './alarm-status-card';
import { AlarmControlsCard } from './alarm-controls-card';
import { UserUidCard } from './user-uid-card';
import { Skeleton } from '../ui/skeleton';
import { useToast } from '@/hooks/use-toast';

export interface AlarmState {
  isArmed: boolean;
  soundDetected: boolean;
  alarmDuration: number;
  lastArmedTimestamp?: Timestamp;
}

export function Dashboard() {
  const firestore = useFirestore();
  const { user, isUserLoading } = useUser();
  const router = useRouter();
  const { toast } = useToast();

  useEffect(() => {
    if (!isUserLoading && !user) {
      router.push('/login');
    }
  }, [isUserLoading, user, router]);

  const alarmStateRef = useMemo(
    () => (firestore && user ? doc(firestore, `users/${user.uid}/soundAlarm/state`) : null),
    [firestore, user]
  );

  const {
    data: alarmState,
    isLoading: isAlarmStateLoading,
    error,
  } = useDoc<AlarmState>(alarmStateRef);

  // Effect for triggering toast notification
  useEffect(() => {
    if (alarmState?.isArmed && alarmState?.soundDetected) {
      toast({
        title: '¡Alarma Activada!',
        description: 'Se ha detectado un sonido fuerte.',
      });
    }
  }, [alarmState?.soundDetected, alarmState?.isArmed, toast]);


  if (isUserLoading || (user && isAlarmStateLoading && alarmState === undefined)) {
    return <DashboardSkeleton />;
  }
  
  if (!user) {
    return <DashboardSkeleton />;
  }

  if (error) {
    return (
      <div className="text-center text-destructive">
        <p>Error al cargar el estado de la alarma.</p>
        <p className="text-xs text-muted-foreground">{error.message}</p>
      </div>
    );
  }

  return (
    <div className="container mx-auto grid max-w-4xl grid-cols-1 gap-6 md:grid-cols-3">
      <div className="md:col-span-3">
        <h1 className="text-3xl font-bold tracking-tight text-center mb-6">
          Panel de control de BabyAlarm
        </h1>
      </div>

      <div className="md:col-span-2 grid grid-cols-1 md:grid-cols-2 gap-6">
        <AlarmStatusCard
          isArmed={alarmState?.isArmed ?? false}
          soundDetected={alarmState?.soundDetected ?? false}
          alarmDuration={alarmState?.alarmDuration ?? 0}
          lastArmedTimestamp={alarmState?.lastArmedTimestamp}
        />
        <AlarmControlsCard
          isArmed={alarmState?.isArmed ?? false}
          isDeviceConnected={!!alarmState}
          initialDuration={alarmState?.alarmDuration ?? 10}
        />
      </div>

      <div className="md:col-span-1 flex flex-col gap-6">
        <UserUidCard user={user} isLoading={isUserLoading} />
        {user && !isAlarmStateLoading && !alarmState && (
          <div className="text-center text-muted-foreground border rounded-lg p-4 flex flex-col items-center justify-center h-full">
            <p className="font-bold">Esperando datos del dispositivo...</p>
            <p className="text-xs mt-2">
              Asegúrate de que el ESP32 esté conectado y configurado con tu
              UID de usuario. Interactúa con un control (ej. armar la alarma) para inicializar el estado.
            </p>
          </div>
        )}
      </div>
    </div>
  );
}

function DashboardSkeleton() {
  return (
    <div className="container mx-auto grid max-w-4xl grid-cols-1 gap-6 md:grid-cols-3">
      <div className="md:col-span-3">
        <Skeleton className="h-9 w-3/4 mx-auto mb-6" />
      </div>
      <div className="md:col-span-2 grid grid-cols-1 md:grid-cols-2 gap-6">
        <Skeleton className="h-56 w-full" />
        <Skeleton className="h-56 w-full" />
      </div>
      <div className="md-col-span-1">
        <Skeleton className="h-40 w-full" />
      </div>
    </div>
  );
}
