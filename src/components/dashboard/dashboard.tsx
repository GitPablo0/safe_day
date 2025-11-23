'use client';

import { Thermometer, Droplets, Volume2, Sun, User, Copy } from 'lucide-react';
import SensorCard from './sensor-card';
import LedStatus from './led-status';
import { useUser, useFirestore, useDoc, useMemoFirebase, setDocumentNonBlocking } from '@/firebase';
import SensorSimulator from './sensor-simulator';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';
import { useToast } from '@/hooks/use-toast';
import { Label } from '@/components/ui/label';
import { doc } from 'firebase/firestore';
import MusicPlayerCard from './music-player-card';
import EventCountCard from './event-count-card';

type SensorData = {
    temperature: number;
    humidity: number;
    soundLevel: number;
    lightLevel: number;
    ledStatus: 1 | 2 | 3;
    orangeSoundEvents: number;
    redSoundEvents: number;
    orangeLightEvents: number;
    redLightEvents: number;
    blockTimeRemaining: number;
};

type ThresholdConfiguration = {
    orangeSoundEventCount: number;
    redSoundEventCount: number;
    orangeLightEventCount: number;
    redLightEventCount: number;
};

function UserInfoCard({ uid }: { uid: string }) {
  const { toast } = useToast();
  const copyToClipboard = () => {
    navigator.clipboard.writeText(uid);
    toast({
      title: "¡UID de Dispositivo Copiado!",
      description: "Ahora puedes pegarlo en el código de tu dispositivo.",
    });
  };

  return (
    <Card className="h-full">
      <CardHeader>
        <CardTitle>Sincronización de Dispositivo</CardTitle>
        <CardDescription>
          Tu ID de Usuario único para sincronizar con tu dispositivo de hardware.
        </CardDescription>
      </CardHeader>
      <CardContent className="flex flex-col justify-center gap-4">
        <div className="flex items-center gap-2">
            <User className="h-4 w-4 text-muted-foreground" />
            <Label htmlFor="user-uid" className="text-sm font-medium">Tu UID de Usuario</Label>
        </div>
        <div className="flex gap-2">
            <Input id="user-uid" readOnly value={uid} className="flex-1" />
            <Button variant="outline" size="icon" onClick={copyToClipboard}>
                <Copy className="h-4 w-4" />
            </Button>
        </div>
      </CardContent>
    </Card>
  );
}


export default function Dashboard() {
  const { user, isUserLoading } = useUser();
  const firestore = useFirestore();
  const { toast } = useToast();
  
  const userId = user?.uid;

  const sensorDataRef = useMemoFirebase(() => {
    if (!firestore || !userId) return null;
    return doc(firestore, `users/${userId}/sensorData/latest`);
  }, [firestore, userId]);

  const thresholdConfigRef = useMemoFirebase(() => {
    if (!firestore || !userId) return null;
    return doc(firestore, `users/${userId}/thresholdConfigurations/main`);
  }, [firestore, userId]);

  const { data: sensorData, isLoading: isSensorDataLoading } = useDoc<SensorData>(sensorDataRef);
  const { data: thresholdConfig, isLoading: isConfigLoading } = useDoc<ThresholdConfiguration>(thresholdConfigRef);

  const temperature = sensorData?.temperature ?? 0;
  const humidity = sensorData?.humidity ?? 0;
  const soundLevel = sensorData?.soundLevel ?? 0;
  const lightLevel = sensorData?.lightLevel ?? 0;
  const ledStatus = sensorData?.ledStatus ?? 1;
  const blockTimeRemaining = sensorData?.blockTimeRemaining ?? 0;

  
  const handleSendSampleData = (data: { temperature: number; humidity: number; soundLevel: number; lightLevel: number }) => {
    if (sensorDataRef) {
      setDocumentNonBlocking(sensorDataRef, { ...data, ledStatus: 1 }, { merge: true });
       toast({
        title: "Datos de Muestra Enviados a Firebase",
        description: `Temp: ${data.temperature}°C, Sonido: ${data.soundLevel}, Luz: ${data.lightLevel}`,
    });
    } else {
        toast({
            title: "Error al enviar datos",
            variant: "destructive",
            description: "No se pudo obtener la referencia a la base de datos.",
        });
    }
  };

  const isLoading = isUserLoading || !userId || isSensorDataLoading || isConfigLoading;

  if (isLoading) {
    return <div className="flex items-center justify-center min-h-screen">Cargando Datos...</div>
  }

  return (
    <div className="flex flex-col gap-6">
      <div className="grid gap-6 md:gap-8 grid-cols-1 lg:grid-cols-4">
        <div className="lg:col-span-4 xl:col-span-3">
          <div className="grid gap-6 md:gap-8 grid-cols-1 md:grid-cols-2 xl:grid-cols-4">
            <SensorCard
              icon={<Thermometer className="h-6 w-6 text-destructive" />}
              title="Temperatura"
              value={temperature}
              unit="°C"
            />
            <SensorCard
              icon={<Droplets className="h-6 w-6 text-blue-500" />}
              title="Humedad"
              value={humidity}
              unit="%"
            />
            <SensorCard
              icon={<Volume2 className="h-6 w-6 text-primary" />}
              title="Nivel de Sonido"
              value={soundLevel}
              unit="bruto"
            />
            <SensorCard
              icon={<Sun className="h-6 w-6 text-yellow-400" />}
              title="Nivel de Luz"
              value={lightLevel}
              unit="bruto"
            />
          </div>
           <div className="grid grid-cols-1 md:grid-cols-2 gap-6 md:gap-8 mt-6">
             <MusicPlayerCard />
             <EventCountCard sensorData={sensorData} thresholdConfig={thresholdConfig}/>
           </div>
        </div>
        <div className="lg:col-span-4 xl:col-span-1 grid grid-cols-1 md:grid-cols-2 lg:grid-cols-2 xl:grid-cols-1 gap-6 md:gap-8">
           <LedStatus level={ledStatus} timeRemaining={blockTimeRemaining} />
           <UserInfoCard uid={userId} />
           <SensorSimulator onSendSampleData={handleSendSampleData} />
        </div>
      </div>
    </div>
  );
}
