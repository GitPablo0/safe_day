'use client';

import { doc, Firestore, DocumentData } from 'firebase/firestore';
import { useFirestore, useDoc, useMemoFirebase } from '@/firebase';
import { AlarmStatusCard } from './alarm-status-card';
import { AlarmControlsCard } from './alarm-controls-card';
import { Skeleton } from '../ui/skeleton';

export interface AlarmState {
  isArmed: boolean;
  presenceDetected: boolean;
  alarmDuration: number;
}

export function Dashboard() {
  const firestore = useFirestore();

  const alarmStateRef = useMemoFirebase(() => {
    if (!firestore) return null;
    return doc(firestore, '/alarm/state');
  }, [firestore]);

  const {
    data: alarmState,
    isLoading,
    error,
  } = useDoc<AlarmState>(alarmStateRef);

  if (isLoading) {
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

  if (!alarmState) {
    return (
        <div className="text-center text-muted-foreground">
            <p>Esperando datos del sistema de alarma...</p>
            <p className="text-xs">
                Asegúrate de que el dispositivo esté conectado y enviando información.
            </p>
        </div>
    );
  }

  return (
    <div className="container mx-auto grid max-w-2xl grid-cols-1 gap-6 md:grid-cols-2">
      <div className="md:col-span-2">
        <h1 className="text-3xl font-bold tracking-tight text-center">
          Panel de Control de Alarma
        </h1>
      </div>
      <AlarmStatusCard
        isArmed={alarmState.isArmed}
        presenceDetected={alarmState.presenceDetected}
      />
      <AlarmControlsCard
        isArmed={alarmState.isArmed}
        alarmDuration={alarmState.alarmDuration}
        firestore={firestore}
      />
    </div>
  );
}

function DashboardSkeleton() {
  return (
    <div className="container mx-auto grid max-w-2xl grid-cols-1 gap-6 md:grid-cols-2">
      <div className="md:col-span-2">
        <Skeleton className="h-9 w-3/4 mx-auto" />
      </div>
      <Skeleton className="h-56 w-full" />
      <Skeleton className="h-56 w-full" />
    </div>
  );
}
