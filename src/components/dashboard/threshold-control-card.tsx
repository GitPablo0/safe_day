"use client";

import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from "@/components/ui/card";
import { Label } from "@/components/ui/label";
import { Slider } from "@/components/ui/slider";
import { Input } from "@/components/ui/input";
import { Button } from "@/components/ui/button";
import { useToast } from "@/hooks/use-toast";
import { Save } from "lucide-react";

interface ThresholdControlCardProps {
  soundThresholds: { low: number; medium: number };
  setSoundThresholds: (value: { low: number; medium: number }) => void;
  tempThresholds: { green: number; orange: number };
  setTempThresholds: (value: { green: number; orange: number }) => void;
}

export default function ThresholdControlCard({
  soundThresholds,
  setSoundThresholds,
  tempThresholds,
  setTempThresholds,
}: ThresholdControlCardProps) {
  const { toast } = useToast();

  const handleSave = () => {
    // In a real application, this would trigger a server action to save to a database.
    console.log("Saving configuration:", { soundThresholds, tempThresholds });
    toast({
      title: "Configuration Saved",
      description: "Your new threshold settings have been saved successfully.",
    });
  };

  return (
    <Card>
      <CardHeader>
        <CardTitle>Threshold Configuration</CardTitle>
        <CardDescription>
          Adjust the sensor thresholds to change the sensitivity of the stress level detection.
        </CardDescription>
      </CardHeader>
      <CardContent className="grid gap-8 grid-cols-1 md:grid-cols-2">
        <div className="space-y-6">
          <h3 className="font-semibold text-lg">Sound Levels</h3>
          <div className="grid gap-4">
            <Label htmlFor="sound-low">Medium Threshold (Orange)</Label>
            <div className="flex items-center gap-4">
              <Slider
                id="sound-low"
                min={0}
                max={1000}
                step={10}
                value={[soundThresholds.low]}
                onValueChange={(value) => setSoundThresholds({ ...soundThresholds, low: value[0] })}
                className="flex-1"
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
            <Label htmlFor="sound-medium">High Threshold (Red)</Label>
            <div className="flex items-center gap-4">
              <Slider
                id="sound-medium"
                min={0}
                max={1000}
                step={10}
                value={[soundThresholds.medium]}
                onValueChange={(value) => setSoundThresholds({ ...soundThresholds, medium: value[0] })}
                className="flex-1"
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
        <div className="space-y-6">
          <h3 className="font-semibold text-lg">Temperature Levels (°C)</h3>
          <div className="grid gap-4">
            <Label htmlFor="temp-green">Medium Threshold (Orange)</Label>
            <div className="flex items-center gap-4">
              <Slider
                id="temp-green"
                min={0}
                max={40}
                step={0.5}
                value={[tempThresholds.green]}
                onValueChange={(value) => setTempThresholds({ ...tempThresholds, green: value[0] })}
                className="flex-1"
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
            <Label htmlFor="temp-orange">High Threshold (Red)</Label>
            <div className="flex items-center gap-4">
              <Slider
                id="temp-orange"
                min={0}
                max={40}
                step={0.5}
                value={[tempThresholds.orange]}
                onValueChange={(value) => setTempThresholds({ ...tempThresholds, orange: value[0] })}
                className="flex-1"
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
      </CardContent>
      <CardFooter className="flex justify-end">
        <Button onClick={handleSave}>
          <Save className="mr-2 h-4 w-4" />
          Save Configuration
        </Button>
      </CardFooter>
    </Card>
  );
}
