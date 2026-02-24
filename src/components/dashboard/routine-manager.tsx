
'use client';

import { useState } from 'react';
import { useFirestore } from '@/firebase';
import { doc, deleteDoc, setDoc } from 'firebase/firestore';
import { Button } from '@/components/ui/button';
import { Card, CardContent } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Plus, Trash2, Music, Clock, CheckCircle2, Pencil, X, Check } from 'lucide-react';
import { useToast } from '@/hooks/use-toast';
import { Badge } from '@/components/ui/badge';
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { errorEmitter } from '@/firebase/error-emitter';
import { FirestorePermissionError } from '@/firebase/errors';

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
  const [editingTaskId, setEditingTaskId] = useState<string | null>(null);
  const [taskForm, setTaskForm] = useState({ title: '', time: '', trackNumber: 1 });

  const resetForm = () => {
    setTaskForm({ title: '', time: '', trackNumber: 1 });
    setIsAdding(false);
    setEditingTaskId(null);
  };

  const handleSave = () => {
    if (!taskForm.title || !taskForm.time || !firestore) return;
    
    const id = editingTaskId || crypto.randomUUID();
    const taskRef = doc(firestore, `users/${userId}/routine/${id}`);
    
    const data = {
      ...taskForm,
      id,
      isActive: true,
      trackNumber: Number(taskForm.trackNumber)
    };

    setDoc(taskRef, data, { merge: true }).catch(async (error) => {
      errorEmitter.emit('permission-error', new FirestorePermissionError({
        path: taskRef.path,
        operation: editingTaskId ? 'update' : 'create',
        requestResourceData: data,
      }));
    });

    toast({ 
      title: editingTaskId ? 'Actividad actualizada' : 'Actividad añadida', 
      description: 'La rutina se ha actualizado correctamente.' 
    });
    resetForm();
  };

  const startEditing = (task: Task) => {
    setEditingTaskId(task.id);
    setTaskForm({
      title: task.title,
      time: task.time,
      trackNumber: task.trackNumber
    });
  };

  const deleteTask = (id: string) => {
    if (!firestore) return;
    const taskRef = doc(firestore, `users/${userId}/routine/${id}`);
    deleteDoc(taskRef).catch(async (error) => {
      errorEmitter.emit('permission-error', new FirestorePermissionError({
        path: taskRef.path,
        operation: 'delete',
      }));
    });
    toast({ title: 'Actividad eliminada' });
  };

  return (
    <div className="space-y-4">
      {routine.length === 0 && !isAdding && (
        <Card className="border-dashed border-primary/30">
          <CardContent className="py-10 text-center text-muted-foreground">
            No hay actividades programadas. Empieza creando la rutina diaria.
          </CardContent>
        </Card>
      )}

      {routine.map((task) => (
        <div key={task.id}>
          {editingTaskId === task.id ? (
            <Card className="border-primary bg-primary/5">
              <CardContent className="p-6 space-y-4">
                <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                  <div className="space-y-2">
                    <Label>Nombre</Label>
                    <Input 
                      value={taskForm.title} 
                      onChange={(e) => setTaskForm({...taskForm, title: e.target.value})}
                      className="bg-background"
                    />
                  </div>
                  <div className="space-y-2">
                    <Label>Hora</Label>
                    <Input 
                      type="time" 
                      value={taskForm.time} 
                      onChange={(e) => setTaskForm({...taskForm, time: e.target.value})}
                      className="bg-background"
                    />
                  </div>
                  <div className="space-y-2">
                    <Label>Audio</Label>
                    <Select 
                      value={taskForm.trackNumber.toString()} 
                      onValueChange={(val) => setTaskForm({...taskForm, trackNumber: parseInt(val)})}
                    >
                      <SelectTrigger className="bg-background">
                        <SelectValue />
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
                  <Button variant="outline" size="sm" onClick={resetForm}><X className="size-4 mr-2"/>Cancelar</Button>
                  <Button size="sm" onClick={handleSave}><Check className="size-4 mr-2"/>Guardar Cambios</Button>
                </div>
              </CardContent>
            </Card>
          ) : (
            <Card className="group overflow-hidden transition-all hover:border-primary/50">
              <CardContent className="p-4 flex items-center justify-between">
                <div className="flex items-center gap-4">
                  <div className="bg-primary/10 p-3 rounded-full text-primary">
                    <Clock className="size-5" />
                  </div>
                  <div>
                    <h3 className="font-bold text-lg">{task.title}</h3>
                    <div className="flex flex-wrap gap-2 mt-1">
                      <Badge variant="outline" className="gap-1 border-primary/20">
                        <Clock className="size-3" /> {task.time}
                      </Badge>
                      <Badge variant="secondary" className="gap-1 bg-primary/5">
                        <Music className="size-3" /> {AUDIO_TRACKS.find(t => t.id === task.trackNumber)?.label || `Pista ${task.trackNumber}`}
                      </Badge>
                      {task.lastCompletedAt && (
                        <Badge variant="default" className="bg-green-500 hover:bg-green-600 gap-1">
                          <CheckCircle2 className="size-3" /> Realizado: {task.lastCompletedAt}
                        </Badge>
                      )}
                    </div>
                  </div>
                </div>
                <div className="flex items-center gap-1 opacity-0 group-hover:opacity-100 transition-opacity">
                  <Button 
                    variant="ghost" 
                    size="icon" 
                    className="text-muted-foreground hover:text-primary"
                    onClick={() => startEditing(task)}
                  >
                    <Pencil className="size-4" />
                  </Button>
                  <Button 
                    variant="ghost" 
                    size="icon" 
                    className="text-muted-foreground hover:text-destructive"
                    onClick={() => deleteTask(task.id)}
                  >
                    <Trash2 className="size-4" />
                  </Button>
                </div>
              </CardContent>
            </Card>
          )}
        </div>
      ))}

      {isAdding ? (
        <Card className="border-primary bg-primary/5">
          <CardContent className="p-6 space-y-4">
            <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
              <div className="space-y-2">
                <Label>Nombre de la Actividad</Label>
                <Input 
                  placeholder="Ej: Almuerzo" 
                  value={taskForm.title} 
                  onChange={(e) => setTaskForm({...taskForm, title: e.target.value})}
                  className="bg-background"
                />
              </div>
              <div className="space-y-2">
                <Label>Hora</Label>
                <Input 
                  type="time" 
                  value={taskForm.time} 
                  onChange={(e) => setTaskForm({...taskForm, time: e.target.value})}
                  className="bg-background"
                />
              </div>
              <div className="space-y-2">
                <Label>Instrucción de Audio</Label>
                <Select 
                  value={taskForm.trackNumber.toString()} 
                  onValueChange={(val) => {
                    const track = AUDIO_TRACKS.find(t => t.id.toString() === val);
                    setTaskForm({
                      ...taskForm, 
                      trackNumber: parseInt(val),
                      title: taskForm.title || (track ? track.label : '')
                    });
                  }}
                >
                  <SelectTrigger className="bg-background">
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
              <Button variant="outline" onClick={resetForm}>Cancelar</Button>
              <Button onClick={handleSave}>Guardar Tarea</Button>
            </div>
          </CardContent>
        </Card>
      ) : (
        !editingTaskId && (
          <Button className="w-full h-12 gap-2 border-primary/30 text-primary hover:bg-primary/5" variant="outline" onClick={() => setIsAdding(true)}>
            <Plus className="size-5" /> Añadir a la Rutina
          </Button>
        )
      )}
    </div>
  );
}
