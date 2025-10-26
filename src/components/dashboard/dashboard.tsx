"use client";

import { useState, useEffect, useMemo } from 'react';
import { Thermometer, Droplets, Volume2 } from 'lucide-react';
import SensorCard from './sensor-card';
import ThresholdControlCard from './threshold-control-card';
import LedStatus from './led-status';

type SoundThresholds = { low: number; medium: number };
type TempThresholds = { green: number; orange: number };

export default function Dashboard() {
  // Sensor data state with mock values
  const [temperature, setTemperature] = useState(22.5);
  const [humidity, setHumidity] = useState(55.1);
  const [soundLevel, setSoundLevel] = useState(250);

  // Thresholds state, initialized with Arduino defaults
  const [soundThresholds, setSoundThresholds] = useState<SoundThresholds>({ low: 300, medium: 380 });
  const [tempThresholds, setTempThresholds] = useState<TempThresholds>({ green: 24, orange: 28 });

  // Effect to simulate real-time sensor data
  useEffect(() => {
    const interval = setInterval(() => {
      setTemperature(prev => parseFloat((18 + Math.random() * 15).toFixed(1)));
      setHumidity(prev => parseFloat((40 + Math.random() * 30).toFixed(1)));
      setSoundLevel(prev => Math.floor(Math.random() * 500));
    }, 3000); // Update every 3 seconds

    return () => clearInterval(interval);
  }, []);

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

  return (
    <div className="grid gap-6 md:gap-8 grid-cols-1 lg:grid-cols-3 xl:grid-cols-4">
      <div className="lg:col-span-3 xl:col-span-4">
        <h1 className="text-2xl font-semibold text-foreground">Dashboard</h1>
        <p className="text-muted-foreground mt-1">
          Monitor sensor data and adjust stress level thresholds in real-time.
        </p>
      </div>

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
      
      <LedStatus level={ledStatus} />
      
      <div className="lg:col-span-3 xl:col-span-4">
        <ThresholdControlCard
          soundThresholds={soundThresholds}
          setSoundThresholds={setSoundThresholds}
          tempThresholds={tempThresholds}
          setTempThresholds={setTempThresholds}
        />
      </div>
    </div>
  );
}
