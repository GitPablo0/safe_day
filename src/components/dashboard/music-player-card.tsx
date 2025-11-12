'use client';

import { useState, useEffect, useCallback, useMemo } from 'react';
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Slider } from "@/components/ui/slider";
import { Play, Pause, Rewind, FastForward, Volume2, Music, Loader2 } from "lucide-react";
import { useUser, useFirestore, useDoc, useMemoFirebase, setDocumentNonBlocking } from '@/firebase';
import { doc } from 'firebase/firestore';
import { useToast } from '@/hooks/use-toast';

type MusicControlState = {
  command?: 'play' | 'pause' | 'next' | 'prev';
  volume?: number;
  lastCommandTimestamp?: number;
  status?: 'playing' | 'paused' | 'stopped';
  currentTrack?: number;
  trackCount?: number;
};

// Debounce function
function debounce<F extends (...args: any[]) => any>(func: F, waitFor: number) {
  let timeout: NodeJS.Timeout;
  return (...args: Parameters<F>): void => {
    clearTimeout(timeout);
    timeout = setTimeout(() => func(...args), waitFor);
  };
}


export default function MusicPlayerCard() {
  const { user } = useUser();
  const firestore = useFirestore();
  const { toast } = useToast();
  
  // This state is for the slider to provide instant UI feedback.
  const [localVolume, setLocalVolume] = useState(15);
  // This state reflects the playing status from Firestore, but can be changed optimistically.
  const [optimisticIsPlaying, setOptimisticIsPlaying] = useState<boolean | null>(null);

  const controlRef = useMemoFirebase(() => {
    if (!firestore || !user?.uid) return null;
    return doc(firestore, `users/${user.uid}/musicControl/state`);
  }, [firestore, user?.uid]);

  const { data: playerState, isLoading: isStateLoading } = useDoc<MusicControlState>(controlRef);

  // Synchronize local UI state with data from Firestore when it changes.
  useEffect(() => {
    if (playerState?.volume !== undefined && !isStateLoading) {
      setLocalVolume(playerState.volume);
    }
    if (playerState?.status !== undefined && !isStateLoading) {
        setOptimisticIsPlaying(playerState.status === 'playing');
    }
  }, [playerState, isStateLoading]);


  const sendCommand = (command: MusicControlState['command']) => {
    if (controlRef) {
      // Optimistic UI update
      if (command === 'play') setOptimisticIsPlaying(true);
      if (command === 'pause') setOptimisticIsPlaying(false);
      
      const payload = { 
        command, 
        lastCommandTimestamp: Date.now() 
      };
      setDocumentNonBlocking(controlRef, payload, { merge: true });
    } else {
        toast({ title: "Error", description: "No se puede enviar el comando.", variant: "destructive" });
    }
  };

  // Memoized debounced function to send volume updates to Firebase
  const debouncedSetVolume = useMemo(
    () =>
      debounce((volume: number) => {
        if (controlRef) {
          setDocumentNonBlocking(controlRef, { volume }, { merge: true });
        }
      }, 100),
    [controlRef]
  );

  const handleVolumeChange = (value: number[]) => {
    const newVolume = value[0];
    setLocalVolume(newVolume); // Update UI instantly
    debouncedSetVolume(newVolume); // Send to Firebase after a delay
  };
  
  const isActuallyPlaying = optimisticIsPlaying ?? playerState?.status === 'playing';
  const trackInfo = (playerState?.trackCount ?? 0) > 0
    ? `Pista ${playerState?.currentTrack ?? '?'} de ${playerState?.trackCount ?? '?'}`
    : (isStateLoading ? 'Cargando...' : 'Sin pistas');


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
          <span className="font-semibold min-w-[120px] text-center">{trackInfo}</span>
        </div>

        <div className="flex items-center justify-center gap-4">
          <Button variant="ghost" size="icon" onClick={() => sendCommand('prev')} disabled={isStateLoading && optimisticIsPlaying === null}>
            <Rewind className="h-6 w-6" />
          </Button>
          <Button 
            variant="default" 
            size="icon" 
            onClick={() => sendCommand(isActuallyPlaying ? 'pause' : 'play')} 
            className="h-14 w-14" 
            disabled={isStateLoading && optimisticIsPlaying === null}
          >
            {isStateLoading && optimisticIsPlaying === null ? <Loader2 className="h-6 w-6 animate-spin"/> : (isActuallyPlaying ? <Pause className="h-8 w-8" /> : <Play className="h-8 w-8" />)}
          </Button>
          <Button variant="ghost" size="icon" onClick={() => sendCommand('next')} disabled={isStateLoading && optimisticIsPlaying === null}>
            <FastForward className="h-6 w-6" />
          </Button>
        </div>

        <div className="flex items-center gap-4">
          <Volume2 className="h-5 w-5 text-muted-foreground" />
          <Slider
            min={0}
            max={30}
            step={1}
            value={[localVolume]}
            onValueChange={handleVolumeChange}
            disabled={isStateLoading && optimisticIsPlaying === null}
          />
        </div>
      </CardContent>
    </Card>
  );
}
