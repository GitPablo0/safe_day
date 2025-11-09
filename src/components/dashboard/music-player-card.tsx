'use client';

import { useState } from 'react';
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Slider } from "@/components/ui/slider";
import { Play, Pause, Rewind, FastForward, Volume2, Music } from "lucide-react";
import { useUser, useFirestore, useDoc, useMemoFirebase, setDocumentNonBlocking } from '@/firebase';
import { doc } from 'firebase/firestore';

type MusicControlState = {
  command?: 'play' | 'pause' | 'next' | 'prev';
  volume?: number;
  status?: 'playing' | 'paused';
  currentTrack?: number;
  trackCount?: number;
};

export default function MusicPlayerCard() {
  const { user } = useUser();
  const firestore = useFirestore();
  const [desiredVolume, setDesiredVolume] = useState(15);

  const controlRef = useMemoFirebase(() => {
    if (!firestore || !user?.uid) return null;
    return doc(firestore, `users/${user.uid}/musicControl/state`);
  }, [firestore, user?.uid]);

  const { data: playerState, isLoading } = useDoc<MusicControlState>(controlRef);

  const sendCommand = (command: MusicControlState['command']) => {
    if (controlRef) {
      setDocumentNonBlocking(controlRef, { command, lastCommandTimestamp: Date.now() }, { merge: true });
    }
  };

  const handleVolumeChange = (value: number[]) => {
    setDesiredVolume(value[0]);
  };

  const handleVolumeCommit = (value: number[]) => {
    if (controlRef) {
      setDocumentNonBlocking(controlRef, { volume: value[0] }, { merge: true });
    }
  };

  const isPlaying = playerState?.status === 'playing';
  const trackInfo = playerState?.currentTrack && playerState?.trackCount 
    ? `Pista ${playerState.currentTrack} de ${playerState.trackCount}` 
    : 'Pista Desconocida';

  return (
    <Card>
      <CardHeader>
        <CardTitle>Control de Música</CardTitle>
        <CardDescription>
          Controla la reproducción de música en tu dispositivo.
        </CardDescription>
      </CardHeader>
      <CardContent className="flex flex-col gap-6">
        <div className="flex items-center justify-center gap-4">
          <Music className="h-6 w-6 text-muted-foreground" />
          <span className="font-semibold">{isLoading ? 'Cargando...' : trackInfo}</span>
        </div>

        <div className="flex items-center justify-center gap-4">
          <Button variant="ghost" size="icon" onClick={() => sendCommand('prev')} disabled={isLoading}>
            <Rewind className="h-6 w-6" />
          </Button>
          <Button variant="default" size="icon" onClick={() => sendCommand(isPlaying ? 'pause' : 'play')} className="h-14 w-14" disabled={isLoading}>
            {isPlaying ? <Pause className="h-8 w-8" /> : <Play className="h-8 w-8" />}
          </Button>
          <Button variant="ghost" size="icon" onClick={() => sendCommand('next')} disabled={isLoading}>
            <FastForward className="h-6 w-6" />
          </Button>
        </div>

        <div className="flex items-center gap-4">
          <Volume2 className="h-5 w-5 text-muted-foreground" />
          <Slider
            min={0}
            max={30}
            step={1}
            value={[desiredVolume]}
            onValueChange={handleVolumeChange}
            onValueCommit={handleVolumeCommit}
            disabled={isLoading}
          />
        </div>
      </CardContent>
    </Card>
  );
}
