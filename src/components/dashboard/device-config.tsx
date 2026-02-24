
'use client';

import { useFirestore, useUser, useDoc, useMemoFirebase } from '@/firebase';
import { doc, setDoc } from 'firebase/firestore';
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card';
import { Label } from '@/components/ui/label';
import { Slider } from '@/components/ui/slider';
import { Speaker, Volume2 } from 'lucide-react';
import { errorEmitter } from '@/firebase/error-emitter';
import { FirestorePermissionError } from '@/firebase/errors';

export function DeviceConfig({ userId }: { userId: string }) {
  const firestore = useFirestore();
  
  const settingsRef = useMemoFirebase(() => {
    if (!firestore || !userId) return null;
    return doc(firestore, `users/${userId}/settings/device`);
  }, [firestore, userId]);

  const { data: settings } = useDoc(settingsRef);

  const handleVolumeChange = (val: number[]) => {
    if (!settingsRef) return;
    const data = { volume: val[0] };
    setDoc(settingsRef, data, { merge: true }).catch(async (error) => {
      errorEmitter.emit('permission-error', new FirestorePermissionError({
        path: settingsRef.path,
        operation: 'update',
        requestResourceData: data,
      }));
    });
  };

  return (
    <Card>
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <Speaker className="size-5" />
          <span>Dispositivo</span>
        </CardTitle>
        <CardDescription>Configuración del parlante físico.</CardDescription>
      </CardHeader>
      <CardContent className="space-y-6">
        <div className="space-y-4">
          <div className="flex justify-between items-center">
            <Label className="flex items-center gap-2">
              <Volume2 className="size-4" /> Volumen
            </Label>
            <span className="text-sm font-bold text-primary">{settings?.volume ?? 15} / 30</span>
          </div>
          <Slider 
            min={0} 
            max={30} 
            step={1} 
            value={[settings?.volume ?? 15]} 
            onValueChange={handleVolumeChange}
          />
        </div>
      </CardContent>
    </Card>
  );
}
