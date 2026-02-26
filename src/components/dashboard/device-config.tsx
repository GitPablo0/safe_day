
'use client';

import { useFirestore, useUser, useDoc, useMemoFirebase } from '@/firebase';
import { doc } from 'firebase/firestore';
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { Speaker, Wifi } from 'lucide-react';

export function DeviceConfig({ userId }: { userId: string }) {
  const firestore = useFirestore();
  
  const settingsRef = useMemoFirebase(() => {
    if (!firestore || !userId) return null;
    return doc(firestore, `users/${userId}/settings/device`);
  }, [firestore, userId]);

  const { data: settings } = useDoc(settingsRef);

  return (
    <Card className="border-primary/10">
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <Speaker className="size-5" />
          <span>Dispositivo</span>
        </CardTitle>
        <CardDescription>Estado del parlante SafeDay.</CardDescription>
      </CardHeader>
      <CardContent className="space-y-6">
        <div className="flex items-center justify-between">
            <div className="flex items-center gap-2">
                <Wifi className="size-4 text-muted-foreground" />
                <span className="text-sm">Estado</span>
            </div>
            <Badge variant={settings?.status === 'online' ? 'default' : 'secondary'}>
                {settings?.status === 'online' ? 'En línea' : 'Desconectado'}
            </Badge>
        </div>
      </CardContent>
    </Card>
  );
}
