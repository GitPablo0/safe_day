'use client';

import { useState, useEffect } from 'react';
import { Thermometer, Droplets, Volume2, Sun, User, Copy, Info } from 'lucide-react';
import SensorCard from './sensor-card';
import ThresholdControlCard from './threshold-control-card';
import LedStatus from './led-status';
import { useUser } from '@/firebase';
import SensorSimulator from './sensor-simulator';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';
import { useToast } from '@/hooks/use-toast';
import { Label } from '@/components/ui/label';
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert';

type SoundThresholds = { low: number; medium: number };
type LightThresholds = { low: number; medium: number };
type TempThresholds = { green: number; orange: number };

type EventLogicConfig = {
  orangeEventCount: number;
  redEventCount: number;
  timeBetweenEvents: number;
  orangeBlockDuration: number;
  redBlockDuration: number;
};

// Mock data to ensure the UI is always functional for presentation
const MOCK_SENSOR_DATA = {
    temperature: 22.5,
    humidity: 45.1,
    soundLevel: 120,
    lightLevel: 350,
    ledStatus: 1 as 1 | 2 | 3,
};

const MOCK_THRESHOLD_CONFIG = {
    soundThresholdLow: 300,
    soundThresholdMedium: 380,
    lightThresholdLow: 400,
    lightThresholdMedium: 600,
    temperatureThresholdVerde: 24,
    temperatureThresholdNaranja: 28,
    orangeSoundEventCount: 3,
    redSoundEventCount: 3,
    timeBetweenEvents: 10000,
    orangeBlockDuration: 60000,
    redBlockDuration: 60000,
    orangeLightEventCount: 3,
    redLightEventCount: 3,
    timeBetweenLightEvents: 10000,
    orangeLightBlockDuration: 60000,
    redLightBlockDuration: 60000,
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

  // Use mock data for presentation
  const sensorData = MOCK_SENSOR_DATA;
  const activeThresholdConfig = MOCK_THRESHOLD_CONFIG;

  // Sensor data state with mock values as fallback
  const temperature = sensorData?.temperature ?? 0;
  const humidity = sensorData?.humidity ?? 0;
  const soundLevel = sensorData?.soundLevel ?? 0;
  const lightLevel = sensorData?.lightLevel ?? 0;
  const ledStatus = sensorData?.ledStatus ?? 1;

  const [soundThresholds, setSoundThresholds] = useState<SoundThresholds>({ low: 300, medium: 380 });
  const [lightThresholds, setLightThresholds] = useState<LightThresholds>({ low: 400, medium: 600 });
  const [tempThresholds, setTempThresholds] = useState<TempThresholds>({ green: 24, orange: 28 });
  const [soundEventLogic, setSoundEventLogic] = useState<EventLogicConfig>({ orangeEventCount: 3, redEventCount: 3, timeBetweenEvents: 10000, orangeBlockDuration: 60000, redBlockDuration: 60000 });
  const [lightEventLogic, setLightEventLogic] = useState<EventLogicConfig>({ orangeEventCount: 3, redEventCount: 3, timeBetweenEvents: 10000, orangeBlockDuration: 60000, redBlockDuration: 60000 });

  useEffect(() => {
    if (activeThresholdConfig) {
      setSoundThresholds({ low: activeThresholdConfig.soundThresholdLow, medium: activeThresholdConfig.soundThresholdMedium });
      setLightThresholds({ low: activeThresholdConfig.lightThresholdLow, medium: activeThresholdConfig.lightThresholdMedium });
      setTempThresholds({ green: activeThresholdConfig.temperatureThresholdVerde, orange: activeThresholdConfig.temperatureThresholdNaranja });
      setSoundEventLogic({
        orangeEventCount: activeThresholdConfig.orangeSoundEventCount,
        redEventCount: activeThresholdConfig.redSoundEventCount,
        timeBetweenEvents: activeThresholdConfig.timeBetweenEvents,
        orangeBlockDuration: activeThresholdConfig.orangeBlockDuration,
        redBlockDuration: activeThresholdConfig.redBlockDuration,
      });
      setLightEventLogic({
        orangeEventCount: activeThresholdConfig.orangeLightEventCount,
        redEventCount: activeThresholdConfig.redLightEventCount,
        timeBetweenEvents: activeThresholdConfig.timeBetweenLightEvents,
        orangeBlockDuration: activeThresholdConfig.orangeLightBlockDuration,
        redBlockDuration: activeThresholdConfig.redLightBlockDuration,
      });
    }
  }, [activeThresholdConfig]);
  
  const handleSaveConfiguration = () => {
    toast({
      title: "Modo de Demostración",
      description: "El guardado está deshabilitado mientras los datos en vivo están pausados.",
    });
  };

  const handleSendSampleData = (data: { temperature: number; humidity: number; soundLevel: number; lightLevel: number }) => {
     toast({
        title: "Modo de Demostración",
        description: `La generación de datos de muestra está deshabilitada. Mostrando datos estáticos.`,
    });
  };

  if (isUserLoading || !userId) {
    return <div className="flex items-center justify-center min-h-screen">Cargando Datos de Usuario...</div>
  }

  return (
    <div className="grid gap-6 md:gap-8 grid-cols-1 lg:grid-cols-4">
      <div className="lg:col-span-4">
        <Alert>
          <Info className="h-4 w-4" />
          <AlertTitle>Modo de Demostración</AlertTitle>
          <AlertDescription>
            Conexión de datos en tiempo real pausada para la demostración. La interfaz es completamente funcional.
          </AlertDescription>
        </Alert>
      </div>

      <div className="lg:col-span-4 mt-6">
        <h1 className="text-2xl font-semibold text-foreground">Panel de Control</h1>
        <p className="text-muted-foreground mt-1">
          Monitorea datos de sensores y ajusta los umbrales de nivel de estrés en tiempo real.
        </p>
      </div>
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
            icon={<Volume2 className="h-6 w-6 text-amber-500" />}
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
      
      <div className="lg:col-span-4">
        <ThresholdControlCard
          soundThresholds={soundThresholds}
          setSoundThresholds={setSoundThresholds}
          lightThresholds={lightThresholds}
          setLightThresholds={setLightThresholds}
          tempThresholds={tempThresholds}
          setTempThresholds={setTempThresholds}
          soundEventLogic={soundEventLogic}
          setSoundEventLogic={setSoundEventLogic}
          lightEventLogic={lightEventLogic}
          setLightEventLogic={setLightEventLogic}
          onSave={handleSaveConfiguration}
        />
      </div>
    </div>
  );
}
