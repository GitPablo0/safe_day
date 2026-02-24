
'use client';

import { useState } from 'react';
import { useFirestore } from '@/firebase';
import { doc, deleteDoc, setDoc } from 'firebase/firestore';
import { Button } from '@/components/ui/button';
import { Card, CardContent } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Plus, Trash2, Music, Clock, CheckCircle2 } from 'lucide-react';
import { useToast } from '@/hooks/use-toast';
import { Badge } from '@/components/ui/badge';
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";

interface Task {
  id: string;
  title: string;
  time: string;
  trackNumber: number;
  isActive: boolean;
  lastCompletedAt?: string;
}

const AUDIO_TRACKS = [
  { id: 1, label: "Tomar pastillas" },
  { id: 2, label: "Desayunar" },
  { id: 3, label: "Almorzar" },
  { id: 4, label: "Cenar" },
  { id: 5, label: "Tomar agua" },
  { id: 6, label: "Bañarse" },
  { id: 7, label: "Cepillar dientes" },
  { id: 8, label: "Vestirse" },
  { id: 9, label: "Descansar / Dormir" },
];

export function RoutineManager({ routine, userId }: { routine: Task[], userId: string }) {
  const firestore = useFirestore();
  const { toast } = useToast();
  const [isAdding, setIsAdding] = useState(false);
  const [newTask, setNewTask] = useState({ title: '', time: '', trackNumber: 1 });

  const addTask = async () => {
    if (!newTask.title || !newTask.time) return;
    const id = crypto.randomUUID();
    const taskRef = doc(firestore, `users/${userId}/routine/${id}`);
    
    await setDoc(taskRef, {
      ...newTask,
      id,
      isActive: true,
      trackNumber: Number(newTask.trackNumber)
    });

    setNewTask({ title: '', time: '', trackNumber: 1 });
    setIsAdding(false);
    toast({ title: 'Actividad añadida', description: 'La rutina se ha actualizado.' });
  };

  const deleteTask = async (id: string) => {
    await deleteDoc(doc(firestore, `users/${userId}/routine/${id}`));
    toast({ title: 'Actividad eliminada' });
  };

  return (
    <div className="space-y-4">
      {routine.length === 0 && !isAdding && (
        <Card className="border-dashed">
          <CardContent className="py-10 text-center text-muted-foreground">
            No hay actividades programadas. Empieza creando la rutina diaria.
          </CardContent>
        </Card>
      )}

      {routine.map((task) => (
        <Card key={task.id} className="group overflow-hidden transition-all hover:border-primary/50">
          <CardContent className="p-4 flex items-center justify-between">
            <div className="flex items-center gap-4">
              <div className="bg-primary/10 p-3 rounded-full text-primary">
                <Clock className="size-5" />
              </div>
              <div>
                <h3 className="font-bold text-lg">{task.title}</h3>
                <div className="flex flex-wrap gap-2 mt-1">
                  <Badge variant="outline" className="gap-1">
                    <Clock className="size-3" /> {task.time}
                  </Badge>
                  <Badge variant="secondary" className="gap-1">
                    <Music className="size-3" /> {AUDIO_TRACKS.find(t => t.id === task.trackNumber)?.label || `Pista ${task.trackNumber}`}
                  </Badge>
                  {task.lastCompletedAt && (
                    <Badge variant="default" className="bg-green-500 hover:bg-green-600 gap-1">
                      <CheckCircle2 className="size-3" /> {task.lastCompletedAt}
                    </Badge>
                  )}
                </div>
              </div>
            </div>
            <Button 
              variant="ghost" 
              size="icon" 
              className="text-muted-foreground hover:text-destructive opacity-0 group-hover:opacity-100 transition-opacity"
              onClick={() => deleteTask(task.id)}
            >
              <Trash2 className="size-5" />
            </Button>
          </CardContent>
        </Card>
      ))}

      {isAdding ? (
        <Card className="border-primary">
          <CardContent className="p-6 space-y-4">
            <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
              <div className="space-y-2">
                <Label>Nombre de la Actividad</Label>
                <Input 
                  placeholder="Ej: Almuerzo" 
                  value={newTask.title} 
                  onChange={(e) => setNewTask({...newTask, title: e.target.value})}
                />
              </div>
              <div className="space-y-2">
                <Label>Hora</Label>
                <Input 
                  type="time" 
                  value={newTask.time} 
                  onChange={(e) => setNewTask({...newTask, time: e.target.value})}
                />
              </div>
              <div className="space-y-2">
                <Label>Instrucción de Audio</Label>
                <Select 
                  value={newTask.trackNumber.toString()} 
                  onValueChange={(val) => setNewTask({...newTask, trackNumber: parseInt(val)})}
                >
                  <SelectTrigger>
                    <SelectValue placeholder="Selecciona un audio" />
                  </SelectTrigger>
                  <SelectContent>
                    {AUDIO_TRACKS.map(track => (
                      <SelectItem key={track.id} value={track.id.toString()}>
                        {track.label}
                      </SelectItem>
                    ))}
                  </SelectContent>
                </Select>
              </div>
            </div>
            <div className="flex justify-end gap-2">
              <Button variant="outline" onClick={() => setIsAdding(false)}>Cancelar</Button>
              <Button onClick={addTask}>Guardar Tarea</Button>
            </div>
          </CardContent>
        </Card>
      ) : (
        <Button className="w-full h-12 gap-2" variant="outline" onClick={() => setIsAdding(true)}>
          <Plus className="size-5" /> Añadir a la Rutina
        </Button>
      )}
    </div>
  );
}
