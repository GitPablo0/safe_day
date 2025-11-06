'use client';

import { useState, useEffect, useMemo } from 'react';
import { Thermometer, Droplets, Volume2, Sun, User, Copy } from 'lucide-react';
import SensorCard from './sensor-card';
import ThresholdControlCard from './threshold-control-card';
import LedStatus from './led-status';
import { useDoc, useUser, useFirestore, useMemoFirebase } from '@/firebase';
import { setDocumentNonBlocking, updateDocumentNonBlocking } from '@/firebase/non-blocking-updates';
import { doc } from 'firebase/firestore';
import SensorSimulator from './sensor-simulator';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';
import { useToast } from '@/hooks/use-toast';
import { Label } from '@/components/ui/label';

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

type FullThresholdConfiguration = {
    id?: string;
    userId: string;
    soundThresholdLow: number;
    soundThresholdMedium: number;
    lightThresholdLow: number;
    lightThresholdMedium: number;
    temperatureThresholdVerde: number;
    temperatureThresholdNaranja: number;
    orangeSoundEventCount: number;
    redSoundEventCount: number;
    timeBetweenEvents: number;
    orangeBlockDuration: number;
    redBlockDuration: number;
    orangeLightEventCount: number;
    redLightEventCount: number;
    timeBetweenLightEvents: number;
    orangeLightBlockDuration: number;
    redLightBlockDuration: number;
};


type SensorData = {
    temperature: number;
    humidity: number;
    soundLevel: number;
    lightLevel: number;
    ledStatus: 1 | 2 | 3;
    timestamp: {
        seconds: number;
        nanoseconds: number;
    }
}

// Default configuration to be created for new users
const createDefaultConfig = (userId: string): FullThresholdConfiguration => ({
    userId: userId,
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
});

function UserInfoCard({ uid }: { uid: string }) {
  const { toast } = useToast();
  const copyToClipboard = () => {
    navigator.clipboard.writeText(uid);
    toast({
      title: "Device UID Copied!",
      description: "You can now paste it in your device code.",
    });
  };

  return (
    <Card className="h-full">
      <CardHeader>
        <CardTitle>Device Sync</CardTitle>
        <CardDescription>
          Your unique User ID to sync with your hardware device.
        </CardDescription>
      </CardHeader>
      <CardContent className="flex flex-col justify-center gap-4">
        <div className="flex items-center gap-2">
            <User className="h-4 w-4 text-muted-foreground" />
            <Label htmlFor="user-uid" className="text-sm font-medium">Your User UID</Label>
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
  const firestore = useFirestore();
  const { user, isUserLoading } = useUser();
  const { toast } = useToast();
  
  const userId = user?.uid;

  const sensorDataRef = useMemoFirebase(() => 
    (firestore && userId) ? doc(firestore, `users/${userId}/sensorData`, 'latest') : null, 
    [firestore, userId]
  );
  const { data: sensorData } = useDoc<SensorData>(sensorDataRef);

  const configDocId = "default_config";
  const thresholdConfigRef = useMemoFirebase(() => 
    (firestore && userId) ? doc(firestore, `users/${userId}/thresholdConfigurations`, configDocId) : null,
    [firestore, userId]
  );
  
  const { data: activeThresholdConfig, isLoading: isConfigLoading } = useDoc<FullThresholdConfiguration>(thresholdConfigRef);
  
  useEffect(() => {
    if (!isUserLoading && userId && !isConfigLoading && thresholdConfigRef && !activeThresholdConfig) {
      console.log(`No config found for user ${userId}. Creating default config.`);
      const defaultConfig = createDefaultConfig(userId);
      setDocumentNonBlocking(thresholdConfigRef, defaultConfig, { merge: true });
    }
  }, [isUserLoading, userId, isConfigLoading, activeThresholdConfig, thresholdConfigRef]);


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
    if (thresholdConfigRef) {
        const updatedData = {
          soundThresholdLow: soundThresholds.low,
          soundThresholdMedium: soundThresholds.medium,
          lightThresholdLow: lightThresholds.low,
          lightThresholdMedium: lightThresholds.medium,
          temperatureThresholdVerde: tempThresholds.green,
          temperatureThresholdNaranja: tempThresholds.orange,
          orangeSoundEventCount: soundEventLogic.orangeEventCount,
          redSoundEventCount: soundEventLogic.redEventCount,
          timeBetweenEvents: soundEventLogic.timeBetweenEvents,
          orangeBlockDuration: soundEventLogic.orangeBlockDuration,
          redBlockDuration: soundEventLogic.redBlockDuration,
          orangeLightEventCount: lightEventLogic.orangeEventCount,
          redLightEventCount: lightEventLogic.redEventCount,
          timeBetweenLightEvents: lightEventLogic.timeBetweenEvents,
          orangeLightBlockDuration: lightEventLogic.orangeBlockDuration,
          redLightBlockDuration: lightEventLogic.redBlockDuration,
        };
        updateDocumentNonBlocking(thresholdConfigRef, updatedData);
        toast({
          title: "Configuration Saved",
          description: "Your new threshold settings have been saved successfully.",
        });
    } else {
       toast({
        variant: "destructive",
        title: "Error",
        description: "Could not save configuration. User or settings not found.",
      });
    }
  };

  const handleSendSampleData = (data: { temperature: number; humidity: number; soundLevel: number; lightLevel: number }) => {
    if (sensorDataRef) {
        const dataWithTimestamp = {
            ...data,
            ledStatus: 1, // Simulator doesn't calculate this, device does.
            timestamp: new Date()
        }
      setDocumentNonBlocking(sensorDataRef, dataWithTimestamp, { merge: true });
    }
  };

  if (isUserLoading || !userId) {
    return <div className="flex items-center justify-center min-h-screen">Loading User Data...</div>
  }

  return (
    <div className="grid gap-6 md:gap-8 grid-cols-1 lg:grid-cols-4">
      <div className="lg:col-span-4">
        <h1 className="text-2xl font-semibold text-foreground">Dashboard</h1>
        <p className="text-muted-foreground mt-1">
          Monitor sensor data and adjust stress level thresholds in real-time.
        </p>
      </div>
      <div className="lg:col-span-4 xl:col-span-3">
        <div className="grid gap-6 md:gap-8 grid-cols-1 md:grid-cols-2 xl:grid-cols-4">
          <SensorCard
            icon={<Thermometer className="h-6 w-6 text-destructive" />}
            title="Temperature"
            value={temperature}
            unit="°C"
          />
          <SensorCard
            icon={<Droplets className="h-6 w-6 text-blue-500" />}
            title="Humidity"
            value={humidity}
            unit="%"
          />
          <SensorCard
            icon={<Volume2 className="h-6 w-6 text-amber-500" />}
            title="Sound Level"
            value={soundLevel}
            unit="raw"
          />
          <SensorCard
            icon={<Sun className="h-6 w-6 text-yellow-400" />}
            title="Light Level"
            value={lightLevel}
            unit="raw"
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
