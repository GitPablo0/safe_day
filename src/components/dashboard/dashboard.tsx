
'use client';

import { useEffect } from 'react';
import { useRouter } from 'next/navigation';
import { useUser, useFirestore, useMemoFirebase } from '@/firebase';
import { collection, query, orderBy } from 'firebase/firestore';
import { useCollection } from '@/firebase/firestore/use-collection';

import { RoutineManager } from './routine-manager';
import { UserUidCard } from './user-uid-card';
import { Skeleton } from '../ui/skeleton';
import { ClipboardList, HeartPulse } from 'lucide-react';

export function Dashboard() {
  const { user, isUserLoading } = useUser();
  const firestore = useFirestore();
  const router = useRouter();

  useEffect(() => {
    if (!isUserLoading && !user) {
      router.push('/login');
    }
  }, [isUserLoading, user, router]);

  const routineQuery = useMemoFirebase(() => {
    if (!firestore || !user) return null;
    return query(
      collection(firestore, `users/${user.uid}/routine`),
      orderBy('time', 'asc')
    );
  }, [firestore, user]);

  const { data: routine, isLoading: isRoutineLoading } = useCollection(routineQuery);

  if (isUserLoading || (user && isRoutineLoading)) {
    return <DashboardSkeleton />;
  }

  if (!user) return null;

  return (
    <div className="container mx-auto max-w-5xl py-8 px-4">
      <header className="mb-10 text-center">
        <div className="flex items-center justify-center gap-2 mb-2 text-primary">
          <HeartPulse className="size-8" />
          <h1 className="text-4xl font-bold tracking-tight">SafeDay</h1>
        </div>
        <p className="text-muted-foreground italic">Cuidando los recuerdos, paso a paso.</p>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-8">
        <div className="lg:col-span-2 space-y-8">
          <section>
            <div className="flex items-center gap-2 mb-4">
              <ClipboardList className="size-6 text-primary" />
              <h2 className="text-2xl font-semibold">Rutina del Paciente</h2>
            </div>
            <RoutineManager routine={routine || []} userId={user.uid} />
          </section>
        </div>

        <div className="space-y-6">
          <UserUidCard user={user} isLoading={isUserLoading} />
        </div>
      </div>
    </div>
  );
}

function DashboardSkeleton() {
  return (
    <div className="container mx-auto max-w-5xl py-12 px-4 space-y-8">
      <Skeleton className="h-12 w-48 mx-auto" />
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-8">
        <div className="lg:col-span-2 space-y-6">
          <Skeleton className="h-[400px] w-full" />
        </div>
        <div className="space-y-6">
          <Skeleton className="h-40 w-full" />
        </div>
      </div>
    </div>
  );
}
