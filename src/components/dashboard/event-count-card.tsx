'use client';

import { Card, CardContent, CardHeader, CardTitle, CardDescription } from "@/components/ui/card";
import { Progress } from "@/components/ui/progress";
import { Volume2, Sun } from 'lucide-react';

type SensorData = {
    orangeSoundEvents?: number;
    redSoundEvents?: number;
    orangeLightEvents?: number;
    redLightEvents?: number;
};

type ThresholdConfiguration = {
    orangeSoundEventCount?: number;
    redSoundEventCount?: number;
    orangeLightEventCount?: number;
    redLightEventCount?: number;
};

interface EventCountCardProps {
    sensorData: SensorData | null;
    thresholdConfig: ThresholdConfiguration | null;
}

const CountDisplay = ({ label, icon, current, max }: { label: string; icon: React.ReactNode; current: number; max: number }) => {
    const percentage = max > 0 ? (current / max) * 100 : 0;
    return (
        <div className="space-y-2">
            <div className="flex items-center gap-2 text-sm font-medium">
                {icon}
                <span>{label}</span>
                <span className="ml-auto text-muted-foreground">{current} / {max}</span>
            </div>
            <Progress value={percentage} />
        </div>
    );
};

export default function EventCountCard({ sensorData, thresholdConfig }: EventCountCardProps) {
    const orangeSoundEvents = sensorData?.orangeSoundEvents ?? 0;
    const redSoundEvents = sensorData?.redSoundEvents ?? 0;
    const orangeLightEvents = sensorData?.orangeLightEvents ?? 0;
    const redLightEvents = sensorData?.redLightEvents ?? 0;

    const orangeSoundThreshold = thresholdConfig?.orangeSoundEventCount ?? 3;
    const redSoundThreshold = thresholdConfig?.redSoundEventCount ?? 3;
    const orangeLightThreshold = thresholdConfig?.orangeLightEventCount ?? 3;
    const redLightThreshold = thresholdConfig?.redLightEventCount ?? 3;

    return (
        <Card className="h-full">
            <CardHeader>
                <CardTitle>Conteo de Eventos</CardTitle>
                <CardDescription>Eventos de estrés detectados en la ventana de tiempo actual.</CardDescription>
            </CardHeader>
            <CardContent className="grid grid-cols-1 md:grid-cols-2 gap-6">
                <div className="space-y-4">
                    <h4 className="font-semibold flex items-center gap-2"><Volume2 className="h-5 w-5 text-primary" />Sonido</h4>
                    <CountDisplay label="Naranja" icon={<div className="h-3 w-3 rounded-full bg-orange-400" />} current={orangeSoundEvents} max={orangeSoundThreshold} />
                    <CountDisplay label="Rojo" icon={<div className="h-3 w-3 rounded-full bg-red-500" />} current={redSoundEvents} max={redSoundThreshold} />
                </div>
                <div className="space-y-4">
                    <h4 className="font-semibold flex items-center gap-2"><Sun className="h-5 w-5 text-yellow-400" />Luz</h4>
                    <CountDisplay label="Naranja" icon={<div className="h-3 w-3 rounded-full bg-orange-400" />} current={orangeLightEvents} max={orangeLightThreshold} />
                    <CountDisplay label="Rojo" icon={<div className="h-3 w-3 rounded-full bg-red-500" />} current={redLightEvents} max={redLightThreshold} />
                </div>
            </CardContent>
        </Card>
    );
}
