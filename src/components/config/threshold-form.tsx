'use client';

import { useState, useEffect } from 'react';
import { useUser, useFirestore, useDoc, useMemoFirebase, setDocumentNonBlocking } from '@/firebase';
import { useToast } from '@/hooks/use-toast';
import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from "@/components/ui/card";
import { Label } from "@/components/ui/label";
import { Slider } from "@/components/ui/slider";
import { Input } from "@/components/ui/input";
import { Button } from "@/components/ui/button";
import { Loader2, Save } from "lucide-react";
import { doc } from 'firebase/firestore';

const INITIAL_THRESHOLD_CONFIG = {
    soundThresholdLow: 300,
    soundThresholdMedium: 380,
    lightThresholdLow: 3000,
    lightThresholdMedium: 1000,
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

type ThresholdConfiguration = typeof INITIAL_THRESHOLD_CONFIG;

export default function ThresholdForm() {
  const { user } = useUser();
  const firestore = useFirestore();
  const { toast } = useToast();
  const [isSaving, setIsSaving] = useState(false);
  const [isInitialLoading, setIsInitialLoading] = useState(true);

  const configRef = useMemoFirebase(() => {
    if (!firestore || !user?.uid) return null;
    return doc(firestore, `users/${user.uid}/thresholdConfigurations/main`);
  }, [firestore, user?.uid]);

  const { data: activeThresholdConfig, isLoading: isConfigLoading } = useDoc<ThresholdConfiguration>(configRef);

  const [config, setConfig] = useState<ThresholdConfiguration>(INITIAL_THRESHOLD_CONFIG);

  useEffect(() => {
    if (!isConfigLoading) {
      if (activeThresholdConfig) {
        setConfig(activeThresholdConfig);
      } else {
        setConfig(INITIAL_THRESHOLD_CONFIG);
      }
      setIsInitialLoading(false);
    }
  }, [activeThresholdConfig, isConfigLoading]);
  
  const handleInputChange = (field: keyof ThresholdConfiguration, value: string | number) => {
    setConfig(prev => ({ ...prev, [field]: Number(value) }));
  };
  
  const handleSliderChange = (field: keyof ThresholdConfiguration, value: number[]) => {
      setConfig(prev => ({ ...prev, [field]: value[0] }));
  };

  const handleSaveConfiguration = async () => {
    if (!configRef) {
        toast({
            title: "Error",
            description: "No se puede guardar la configuración. Referencia de usuario no encontrada.",
            variant: "destructive"
        });
        return;
    }
    setIsSaving(true);
    try {
        setDocumentNonBlocking(configRef, config, { merge: true });
        toast({
            title: "Configuración Guardada",
            description: "Tus nuevos umbrales se han guardado en Firebase.",
        });
    } catch (e: any) {
        toast({
            title: "Error al Guardar",
            description: e.message || "No se pudo guardar la configuración en Firebase.",
            variant: "destructive",
        });
    } finally {
        setIsSaving(false);
    }
  };

  if (isInitialLoading) {
      return (
          <div className="flex justify-center items-center h-96">
              <Loader2 className="h-8 w-8 animate-spin text-primary" />
          </div>
      )
  }

  return (
    <Card>
      <CardContent className="grid gap-8 grid-cols-1 md:grid-cols-2 lg:grid-cols-3 pt-6">
        {/* Sound Levels */}
        <div className="space-y-6">
          <h3 className="font-semibold text-lg">Niveles de Sonido</h3>
          <div className="grid gap-4">
            <Label htmlFor="sound-low">Umbral Medio (Naranja)</Label>
            <div className="flex items-center gap-4">
              <Slider
                id="sound-low"
                min={0}
                max={1023}
                step={10}
                value={[config.soundThresholdLow]}
                onValueChange={(value) => handleSliderChange('soundThresholdLow', value)}
              />
              <Input
                type="number"
                value={config.soundThresholdLow}
                onChange={(e) => handleInputChange('soundThresholdLow', e.target.value)}
                className="w-24"
              />
            </div>
          </div>
          <div className="grid gap-4">
            <Label htmlFor="sound-medium">Umbral Alto (Rojo)</Label>
            <div className="flex items-center gap-4">
              <Slider
                id="sound-medium"
                min={0}
                max={1023}
                step={10}
                value={[config.soundThresholdMedium]}
                onValueChange={(value) => handleSliderChange('soundThresholdMedium', value)}
              />
              <Input
                type="number"
                value={config.soundThresholdMedium}
                onChange={(e) => handleInputChange('soundThresholdMedium', e.target.value)}
                className="w-24"
              />
            </div>
          </div>
        </div>

        {/* Light Levels */}
        <div className="space-y-6">
          <h3 className="font-semibold text-lg">Niveles de Luz (Inverso)</h3>
          <div className="grid gap-4">
            <Label htmlFor="light-low">Umbral Medio (Naranja)</Label>
            <div className="flex items-center gap-4">
              <Slider
                id="light-low"
                min={0}
                max={4095}
                step={10}
                value={[config.lightThresholdLow]}
                onValueChange={(value) => handleSliderChange('lightThresholdLow', value)}
              />
              <Input
                type="number"
                value={config.lightThresholdLow}
                onChange={(e) => handleInputChange('lightThresholdLow', e.target.value)}
                className="w-24"
              />
            </div>
          </div>
          <div className="grid gap-4">
            <Label htmlFor="light-medium">Umbral Alto (Rojo)</Label>
            <div className="flex items-center gap-4">
              <Slider
                id="light-medium"
                min={0}
                max={4095}
                step={10}
                value={[config.lightThresholdMedium]}
                onValueChange={(value) => handleSliderChange('lightThresholdMedium', value)}
              />
              <Input
                type="number"
                value={config.lightThresholdMedium}
                onChange={(e) => handleInputChange('lightThresholdMedium', e.target.value)}
                className="w-24"
              />
            </div>
          </div>
        </div>

        {/* Temperature Levels */}
        <div className="space-y-6">
          <h3 className="font-semibold text-lg">Niveles de Temperatura (°C)</h3>
          <div className="grid gap-4">
            <Label htmlFor="temp-green">Umbral Medio (Naranja)</Label>
            <div className="flex items-center gap-4">
              <Slider
                id="temp-green"
                min={0}
                max={40}
                step={0.5}
                value={[config.temperatureThresholdVerde]}
                onValueChange={(value) => handleSliderChange('temperatureThresholdVerde', value)}
              />
              <Input
                type="number"
                value={config.temperatureThresholdVerde}
                onChange={(e) => handleInputChange('temperatureThresholdVerde', e.target.value)}
                className="w-24"
              />
            </div>
          </div>
          <div className="grid gap-4">
            <Label htmlFor="temp-orange">Umbral Alto (Rojo)</Label>
            <div className="flex items-center gap-4">
              <Slider
                id="temp-orange"
                min={0}
                max={40}
                step={0.5}
                value={[config.temperatureThresholdNaranja]}
                onValueChange={(value) => handleSliderChange('temperatureThresholdNaranja', value)}
              />
              <Input
                type="number"
                value={config.temperatureThresholdNaranja}
                onChange={(e) => handleInputChange('temperatureThresholdNaranja', e.target.value)}
                className="w-24"
              />
            </div>
          </div>
        </div>

        {/* Sound Event Logic */}
        <div className="space-y-6">
            <h3 className="font-semibold text-lg">Lógica de Eventos de Sonido</h3>
            <div className="grid grid-cols-2 gap-4">
                <div className="grid gap-2">
                    <Label>Eventos Naranja</Label>
                    <Input type="number" value={config.orangeSoundEventCount} onChange={(e) => handleInputChange('orangeSoundEventCount', e.target.value)} />
                </div>
                <div className="grid gap-2">
                    <Label>Eventos Rojos</Label>
                    <Input type="number" value={config.redSoundEventCount} onChange={(e) => handleInputChange('redSoundEventCount', e.target.value)} />
                </div>
            </div>
            <div className="grid gap-2">
                <Label>Ventana de Tiempo (ms)</Label>
                <Input type="number" value={config.timeBetweenEvents} onChange={(e) => handleInputChange('timeBetweenEvents', e.target.value)} />
            </div>
            <div className="grid grid-cols-2 gap-4">
                <div className="grid gap-2">
                    <Label>Bloqueo Naranja (ms)</Label>
                    <Input type="number" value={config.orangeBlockDuration} onChange={(e) => handleInputChange('orangeBlockDuration', e.target.value)} />
                </div>
                <div className="grid gap-2">
                    <Label>Bloqueo Rojo (ms)</Label>
                    <Input type="number" value={config.redBlockDuration} onChange={(e) => handleInputChange('redBlockDuration', e.target.value)} />
                </div>
            </div>
        </div>

        {/* Light Event Logic */}
        <div className="space-y-6">
            <h3 className="font-semibold text-lg">Lógica de Eventos de Luz</h3>
            <div className="grid grid-cols-2 gap-4">
                <div className="grid gap-2">
                    <Label>Eventos Naranja</Label>
                    <Input type="number" value={config.orangeLightEventCount} onChange={(e) => handleInputChange('orangeLightEventCount', e.target.value)} />
                </div>
                <div className="grid gap-2">
                    <Label>Eventos Rojos</Label>
                    <Input type="number" value={config.redLightEventCount} onChange={(e) => handleInputChange('redLightEventCount', e.target.value)} />
                </div>
            </div>
            <div className="grid gap-2">
                <Label>Ventana de Tiempo (ms)</Label>
                <Input type="number" value={config.timeBetweenLightEvents} onChange={(e) => handleInputChange('timeBetweenLightEvents', e.target.value)} />
            </div>
            <div className="grid grid-cols-2 gap-4">
                <div className="grid gap-2">
                    <Label>Bloqueo Naranja (ms)</Label>
                    <Input type="number" value={config.orangeLightBlockDuration} onChange={(e) => handleInputChange('orangeLightBlockDuration', e.target.value)} />
                </div>
                <div className="grid gap-2">
                    <Label>Bloqueo Rojo (ms)</Label>
                    <Input type="number" value={config.redLightBlockDuration} onChange={(e) => handleInputChange('redLightBlockDuration', e.target.value)} />
                </div>
            </div>
        </div>

      </CardContent>
      <CardFooter className="flex justify-end">
        <Button onClick={handleSaveConfiguration} disabled={isSaving}>
            {isSaving ? (
                <Loader2 className="mr-2 h-4 w-4 animate-spin" />
            ) : (
                <Save className="mr-2 h-4 w-4" />
            )}
          Guardar Configuración
        </Button>
      </CardFooter>
    </Card>
  );
}
