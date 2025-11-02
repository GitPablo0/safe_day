"use client";

import { useState, useEffect, useMemo } from 'react';
import { Thermometer, Droplets, Volume2, User, Copy } from 'lucide-react';
import SensorCard from './sensor-card';
import ThresholdControlCard from './threshold-control-card';
import LedStatus from './led-status';
import { useDoc, useCollection, useUser, useFirestore, useAuth, useMemoFirebase } from '@/firebase';
import { initiateAnonymousSignIn, setDocumentNonBlocking, addDocumentNonBlocking } from '@/firebase';
import { doc, collection, query, limit } from 'firebase/firestore';
import SensorSimulator from './sensor-simulator';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';
import { useToast } from '@/hooks/use-toast';

type SoundThresholds = { low: number; medium: number };
type TempThresholds = { green: number; orange: number };
type ThresholdConfiguration = {
  id: string;
  userId: string;
  soundThresholdLow: number;
  soundThresholdMedium: number;
  temperatureThresholdVerde: number;
  temperatureThresholdNaranja: number;
};

// Default configuration to be created for new users
const createDefaultConfig = (userId: string) => ({
    userId: userId,
    soundThresholdLow: 300,
    soundThresholdMedium: 380,
    temperatureThresholdVerde: 24,
    temperatureThresholdNaranja: 28,
    redSoundEventCount: 5,
    orangeSoundEventCount: 3,
    redBlockDuration: 5000,
    orangeBlockDuration: 3000,
    timeBetweenEvents: 1000,
});


function UserInfoCard({ uid }: { uid: string }) {
  const { toast } = useToast();
  
  const handleCopy = () => {
    navigator.clipboard.writeText(uid);
    toast({
      title: "UID Copied!",
      description: "The User UID has been copied to your clipboard.",
    });
  };

  return (
    <Card className="h-full">
      <CardHeader>
        <CardTitle>Device Sync</CardTitle>
        <CardDescription>
          Use this User UID to sync your hardware device with the dashboard.
        </CardDescription>
      </CardHeader>
      <CardContent className="flex flex-col justify-center gap-4">
          <div className="flex items-center gap-2">
            <User className="h-4 w-4 text-muted-foreground" />
            <Label htmlFor="user-uid" className="text-sm font-medium">User UID</Label>
          </div>
          <div className="flex gap-2">
            <Input id="user-uid" readOnly value={uid} className="flex-1" />
            <Button variant="outline" size="icon" onClick={handleCopy}>
              <Copy className="h-4 w-4" />
            </Button>
          </div>
      </CardContent>
    </Card>
  );
}


export default function Dashboard() {
  const firestore = useFirestore();
  const auth = useAuth();
  const { user, isUserLoading } = useUser();

  useEffect(() => {
    if (!isUserLoading && !user && auth) {
      initiateAnonymousSignIn(auth);
    }
  }, [user, isUserLoading, auth]);

  const sensorDataRef = useMemoFirebase(() => firestore ? doc(firestore, 'sensorData', 'latest') : null, [firestore]);
  const { data: sensorData } = useDoc<{temperature: number, humidity: number, soundLevel: number}>(sensorDataRef);

  // Reference to the user's threshold configurations collection
  const thresholdConfigCollectionRef = useMemoFirebase(() => 
    (firestore && user) ? collection(firestore, `users/${user.uid}/thresholdConfigurations`) : null, 
    [firestore, user]
  );
  
  // Query to get the first (and likely only) configuration document
  const thresholdConfigQuery = useMemoFirebase(() => 
    thresholdConfigCollectionRef ? query(thresholdConfigCollectionRef, limit(1)) : null,
    [thresholdConfigCollectionRef]
  );

  const { data: thresholdConfigurations, isLoading: isConfigLoading } = useCollection<ThresholdConfiguration>(thresholdConfigQuery);
  
  // Effect to create a default configuration if none exists for the user
  useEffect(() => {
    // Check if loading is finished, a user exists, and there are no configurations
    if (!isConfigLoading && user && thresholdConfigCollectionRef && thresholdConfigurations?.length === 0) {
      console.log(`No config found for user ${user.uid}. Creating default.`);
      const defaultConfig = createDefaultConfig(user.uid);
      addDocumentNonBlocking(thresholdConfigCollectionRef, defaultConfig);
    }
  }, [isConfigLoading, user, thresholdConfigurations, thresholdConfigCollectionRef]);


  const activeThresholdConfig = useMemo(() => thresholdConfigurations?.[0], [thresholdConfigurations]);

  // Sensor data state with mock values as fallback
  const temperature = sensorData?.temperature ?? 0;
  const humidity = sensorData?.humidity ?? 0;
  const soundLevel = sensorData?.soundLevel ?? 0;

  // Thresholds state, initialized with Arduino defaults, updated from Firestore
  const [soundThresholds, setSoundThresholds] = useState<SoundThresholds>({ low: 300, medium: 380 });
  const [tempThresholds, setTempThresholds] = useState<TempThresholds>({ green: 24, orange: 28 });

  useEffect(() => {
    if (activeThresholdConfig) {
      setSoundThresholds({ low: activeThresholdConfig.soundThresholdLow, medium: activeThresholdConfig.soundThresholdMedium });
      setTempThresholds({ green: activeThresholdConfig.temperatureThresholdVerde, orange: activeThresholdConfig.temperatureThresholdNaranja });
    }
  }, [activeThresholdConfig]);
  
  // Logic to determine the final LED status
  const ledStatus = useMemo(() => {
    let estadoSonido: 1 | 2 | 3 = 1;
    if (soundLevel >= soundThresholds.low) estadoSonido = 2;
    if (soundLevel >= soundThresholds.medium) estadoSonido = 3;

    let estadoTemp: 1 | 2 | 3 = 1;
    if (temperature >= tempThresholds.green) estadoTemp = 2;
    if (temperature >= tempThresholds.orange) estadoTemp = 3;

    return Math.max(estadoSonido, estadoTemp) as 1 | 2 | 3;
  }, [soundLevel, temperature, soundThresholds, tempThresholds]);

  const handleSaveConfiguration = (newSoundThresholds: SoundThresholds, newTempThresholds: TempThresholds) => {
    if (user && firestore && activeThresholdConfig) {
        // Update existing configuration
        const configRef = doc(firestore, `users/${user.uid}/thresholdConfigurations`, activeThresholdConfig.id);
        const updatedData = {
          soundThresholdLow: newSoundThresholds.low,
          soundThresholdMedium: newSoundThresholds.medium,
          temperatureThresholdVerde: newTempThresholds.green,
          temperatureThresholdNaranja: newTempThresholds.orange,
        };
        setDocumentNonBlocking(configRef, updatedData, { merge: true });
    }
  };

  const handleSendSampleData = (data: { temperature: number; humidity: number; soundLevel: number }) => {
    if (sensorDataRef) {
      setDocumentNonBlocking(sensorDataRef, data, { merge: true });
    }
  };

  if (isUserLoading || !user) {
    return <div className="flex items-center justify-center min-h-screen">Loading...</div>
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
        <div className="grid gap-6 md:gap-8 grid-cols-1 md:grid-cols-3">
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
        </div>
      </div>
      <div className="lg:col-span-4 xl:col-span-1 grid grid-cols-1 md:grid-cols-2 lg:grid-cols-2 xl:grid-cols-1 gap-6 md:gap-8">
         <LedStatus level={ledStatus} />
         {user && <UserInfoCard uid={user.uid} />}
         <SensorSimulator onSendSampleData={handleSendSampleData} />
      </div>
      
      <div className="lg:col-span-4">
        <ThresholdControlCard
          soundThresholds={soundThresholds}
          setSoundThresholds={setSoundThresholds}
          tempThresholds={tempThresholds}
          setTempThresholds={setTempThresholds}
          onSave={handleSaveConfiguration}
        />
      </div>
    </div>
  );
}
