'use client';

import { Thermometer, Droplets, Volume2, Sun, User, Copy, Info } from 'lucide-react';
import SensorCard from './sensor-card';
import LedStatus from './led-status';
import { useUser } from '@/firebase';
import SensorSimulator from './sensor-simulator';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';
import { useToast } from '@/hooks/use-toast';
import { Label } from '@/components/ui/label';

// TODO: Replace with live data from Firebase
const MOCK_SENSOR_DATA = {
    temperature: 0,
    humidity: 0,
    soundLevel: 0,
    lightLevel: 0,
    ledStatus: 1 as 1 | 2 | 3,
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
  const { toast } = useToast();
  
  const userId = user?.uid;

  // TODO: Replace with live data from Firebase
  const sensorData = MOCK_SENSOR_DATA;

  const temperature = sensorData?.temperature ?? 0;
  const humidity = sensorData?.humidity ?? 0;
  const soundLevel = sensorData?.soundLevel ?? 0;
  const lightLevel = sensorData?.lightLevel ?? 0;
  const ledStatus = sensorData?.ledStatus ?? 1;

  
  const handleSendSampleData = (data: { temperature: number; humidity: number; soundLevel: number; lightLevel: number }) => {
     toast({
        title: "Funcionalidad no implementada",
        variant: "destructive",
        description: `La escritura de datos de ejemplo a Firebase aún no está conectada.`,
    });
  };

  if (isUserLoading || !userId) {
    return <div className="flex items-center justify-center min-h-screen">Cargando Datos de Usuario...</div>
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
        </div>
        <div className="lg:col-span-4 xl:col-span-1 grid grid-cols-1 md:grid-cols-2 lg:grid-cols-2 xl:grid-cols-1 gap-6 md:gap-8">
           <LedStatus level={ledStatus} />
           <UserInfoCard uid={userId} />
           <SensorSimulator onSendSampleData={handleSendSampleData} />
        </div>
      </div>
    </div>
  );
}
