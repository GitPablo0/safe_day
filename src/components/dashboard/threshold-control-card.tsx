"use client";

import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from "@/components/ui/card";
import { Label } from "@/components/ui/label";
import { Slider } from "@/components/ui/slider";
import { Input } from "@/components/ui/input";
import { Button } from "@/components/ui/button";
import { Save } from "lucide-react";

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

interface ThresholdControlCardProps {
  soundThresholds: SoundThresholds;
  setSoundThresholds: (value: SoundThresholds) => void;
  lightThresholds: LightThresholds;
  setLightThresholds: (value: LightThresholds) => void;
  tempThresholds: TempThresholds;
  setTempThresholds: (value: TempThresholds) => void;
  soundEventLogic: EventLogicConfig;
  setSoundEventLogic: (value: EventLogicConfig) => void;
  lightEventLogic: EventLogicConfig;
  setLightEventLogic: (value: EventLogicConfig) => void;
  onSave: () => void;
}

export default function ThresholdControlCard({
  soundThresholds,
  setSoundThresholds,
  lightThresholds,
  setLightThresholds,
  tempThresholds,
  setTempThresholds,
  soundEventLogic,
  setSoundEventLogic,
  lightEventLogic,
  setLightEventLogic,
  onSave,
}: ThresholdControlCardProps) {

  const handleSave = () => {
    onSave();
  };

  return (
    <Card>
      <CardHeader>
        <CardTitle>Configuración de Umbrales</CardTitle>
        <CardDescription>
          Ajusta los umbrales de los sensores y la lógica de eventos para cambiar la sensibilidad de la detección de estrés.
        </CardDescription>
      </CardHeader>
      <CardContent className="grid gap-8 grid-cols-1 md:grid-cols-2 lg:grid-cols-3">
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
                value={[soundThresholds.low]}
                onValueChange={(value) => setSoundThresholds({ ...soundThresholds, low: value[0] })}
                className="[&>span>span]:bg-amber-500 [&>span]:bg-amber-500/30"
              />
              <Input
                type="number"
                value={soundThresholds.low}
                onChange={(e) => setSoundThresholds({ ...soundThresholds, low: Number(e.target.value) })}
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
                value={[soundThresholds.medium]}
                onValueChange={(value) => setSoundThresholds({ ...soundThresholds, medium: value[0] })}
                className="[&>span>span]:bg-amber-500 [&>span]:bg-amber-500/30"
              />
              <Input
                type="number"
                value={soundThresholds.medium}
                onChange={(e) => setSoundThresholds({ ...soundThresholds, medium: Number(e.target.value) })}
                className="w-24"
              />
            </div>
          </div>
        </div>

        {/* Light Levels */}
        <div className="space-y-6">
          <h3 className="font-semibold text-lg">Niveles de Luz</h3>
          <div className="grid gap-4">
            <Label htmlFor="light-low">Umbral Medio (Naranja)</Label>
            <div className="flex items-center gap-4">
              <Slider
                id="light-low"
                min={0}
                max={4095}
                step={10}
                value={[lightThresholds.low]}
                onValueChange={(value) => setLightThresholds({ ...lightThresholds, low: value[0] })}
                className="[&>span>span]:bg-yellow-400 [&>span]:bg-yellow-400/30"
              />
              <Input
                type="number"
                value={lightThresholds.low}
                onChange={(e) => setLightThresholds({ ...lightThresholds, low: Number(e.target.value) })}
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
                value={[lightThresholds.medium]}
                onValueChange={(value) => setLightThresholds({ ...lightThresholds, medium: value[0] })}
                className="[&>span>span]:bg-yellow-400 [&>span]:bg-yellow-400/30"
              />
              <Input
                type="number"
                value={lightThresholds.medium}
                onChange={(e) => setLightThresholds({ ...lightThresholds, medium: Number(e.target.value) })}
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
                value={[tempThresholds.green]}
                onValueChange={(value) => setTempThresholds({ ...tempThresholds, green: value[0] })}
                className="[&>span>span]:bg-destructive [&>span]:bg-destructive/30"
              />
              <Input
                type="number"
                value={tempThresholds.green}
                onChange={(e) => setTempThresholds({ ...tempThresholds, green: Number(e.target.value) })}
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
                value={[tempThresholds.orange]}
                onValueChange={(value) => setTempThresholds({ ...tempThresholds, orange: value[0] })}
                className="[&>span>span]:bg-destructive [&>span]:bg-destructive/30"
              />
              <Input
                type="number"
                value={tempThresholds.orange}
                onChange={(e) => setTempThresholds({ ...tempThresholds, orange: Number(e.target.value) })}
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
                    <Input type="number" value={soundEventLogic.orangeEventCount} onChange={(e) => setSoundEventLogic({...soundEventLogic, orangeEventCount: Number(e.target.value)})} />
                </div>
                <div className="grid gap-2">
                    <Label>Eventos Rojos</Label>
                    <Input type="number" value={soundEventLogic.redEventCount} onChange={(e) => setSoundEventLogic({...soundEventLogic, redEventCount: Number(e.target.value)})} />
                </div>
            </div>
            <div className="grid gap-2">
                <Label>Ventana de Tiempo (ms)</Label>
                <Input type="number" value={soundEventLogic.timeBetweenEvents} onChange={(e) => setSoundEventLogic({...soundEventLogic, timeBetweenEvents: Number(e.target.value)})} />
            </div>
            <div className="grid grid-cols-2 gap-4">
                <div className="grid gap-2">
                    <Label>Bloqueo Naranja (ms)</Label>
                    <Input type="number" value={soundEventLogic.orangeBlockDuration} onChange={(e) => setSoundEventLogic({...soundEventLogic, orangeBlockDuration: Number(e.target.value)})} />
                </div>
                <div className="grid gap-2">
                    <Label>Bloqueo Rojo (ms)</Label>
                    <Input type="number" value={soundEventLogic.redBlockDuration} onChange={(e) => setSoundEventLogic({...soundEventLogic, redBlockDuration: Number(e.target.value)})} />
                </div>
            </div>
        </div>

        {/* Light Event Logic */}
        <div className="space-y-6">
            <h3 className="font-semibold text-lg">Lógica de Eventos de Luz</h3>
            <div className="grid grid-cols-2 gap-4">
                <div className="grid gap-2">
                    <Label>Eventos Naranja</Label>
                    <Input type="number" value={lightEventLogic.orangeEventCount} onChange={(e) => setLightEventLogic({...lightEventLogic, orangeEventCount: Number(e.target.value)})} />
                </div>
                <div className="grid gap-2">
                    <Label>Eventos Rojos</Label>
                    <Input type="number" value={lightEventLogic.redEventCount} onChange={(e) => setLightEventLogic({...lightEventLogic, redEventCount: Number(e.target.value)})} />
                </div>
            </div>
            <div className="grid gap-2">
                <Label>Ventana de Tiempo (ms)</Label>
                <Input type="number" value={lightEventLogic.timeBetweenEvents} onChange={(e) => setLightEventLogic({...lightEventLogic, timeBetweenEvents: Number(e.target.value)})} />
            </div>
            <div className="grid grid-cols-2 gap-4">
                <div className="grid gap-2">
                    <Label>Bloqueo Naranja (ms)</Label>
                    <Input type="number" value={lightEventLogic.orangeBlockDuration} onChange={(e) => setLightEventLogic({...lightEventLogic, orangeBlockDuration: Number(e.target.value)})} />
                </div>
                <div className="grid gap-2">
                    <Label>Bloqueo Rojo (ms)</Label>
                    <Input type="number" value={lightEventLogic.redBlockDuration} onChange={(e) => setLightEventLogic({...lightEventLogic, redBlockDuration: Number(e.target.value)})} />
                </div>
            </div>
        </div>

      </CardContent>
      <CardFooter className="flex justify-end">
        <Button onClick={handleSave}>
          <Save className="mr-2 h-4 w-4" />
          Guardar Configuración
        </Button>
      </CardFooter>
    </Card>
  );
}
