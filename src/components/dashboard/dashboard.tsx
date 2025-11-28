'use client';

import { useEffect, useMemo } from 'react';
import { useRouter } from 'next/navigation';
import { useUser, useRtdb } from '@/firebase';
import { useRtdbValue } from '@/firebase/rtdb/use-rtdb-value';
import { ref } from 'firebase/database';

import { AlarmStatusCard } from './alarm-status-card';
import { AlarmControlsCard } from './alarm-controls-card';
import { UserUidCard } from './user-uid-card';
import { Skeleton } from '../ui/skeleton';

export interface AlarmState {
  isArmed: boolean;
  presenceDetected: boolean;
  alarmDuration: number;
}

export function Dashboard() {
  const rtdb = useRtdb();
  const { user, isUserLoading } = useUser();
  const router = useRouter();

  useEffect(() => {
    // If loading is finished and there's no user, redirect to login
    if (!isUserLoading && !user) {
      router.push('/login');
    }
  }, [isUserLoading, user, router]);

  const alarmStateRef = useMemo(
    () => (rtdb ? ref(rtdb, 'alarm/state') : null),
    [rtdb]
  );

  const {
    data: alarmState,
    isLoading: isAlarmStateLoading,
    error,
  } = useRtdbValue<AlarmState>(alarmStateRef);

  // While checking auth state or loading data, show a skeleton
  if (isUserLoading || isAlarmStateLoading) {
    return <DashboardSkeleton />;
  }

  // If there's no user, don't render the dashboard content, the redirect will handle it
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
          Panel de Control de Alarma
        </h1>
      </div>

      <div className="md:col-span-2 grid grid-cols-1 md:grid-cols-2 gap-6">
        <AlarmStatusCard
          isArmed={alarmState?.isArmed ?? false}
          presenceDetected={alarmState?.presenceDetected ?? false}
        />
        <AlarmControlsCard
          isArmed={alarmState?.isArmed ?? false}
          alarmDuration={alarmState?.alarmDuration ?? 10}
          isDeviceConnected={!!alarmState}
        />
      </div>

      <div className="md:col-span-1 flex flex-col gap-6">
        <UserUidCard user={user} isLoading={isUserLoading} />
        {!alarmState && (
          <div className="text-center text-muted-foreground border rounded-lg p-4 flex flex-col items-center justify-center h-full">
            <p className="font-bold">Esperando datos del dispositivo...</p>
            <p className="text-xs mt-2">
              Asegúrate de que el ESP32 esté conectado a internet y
              configurado.
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
      <div className="md:col-span-1">
        <Skeleton className="h-40 w-full" />
      </div>
    </div>
  );
}
