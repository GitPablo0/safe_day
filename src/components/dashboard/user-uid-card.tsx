
'use client';

import { useState } from 'react';
import { User, signOut } from 'firebase/auth';
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Skeleton } from '@/components/ui/skeleton';
import { KeyRound, Copy, Check, LogOut, Info } from 'lucide-react';
import { useAuth } from '@/firebase';
import { Alert, AlertDescription, AlertTitle } from "@/components/ui/alert";

interface UserUidCardProps {
  user: User | null;
  isLoading: boolean;
}

export function UserUidCard({ user, isLoading }: UserUidCardProps) {
  const [copied, setCopied] = useState(false);
  const auth = useAuth();

  const handleCopy = () => {
    if (user?.uid) {
      navigator.clipboard.writeText(user.uid);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    }
  };

  const handleSignOut = async () => {
    await signOut(auth);
  };

  return (
    <Card className="border-primary/20 bg-primary/5">
      <CardHeader>
        <div className="flex justify-between items-start">
            <div>
                <CardTitle className="flex items-center gap-2 text-primary">
                <KeyRound className="size-5" />
                <span>Vinculación</span>
                </CardTitle>
                <CardDescription>
                Identificador para tu dispositivo SafeDay.
                </CardDescription>
            </div>
             <Button
              variant="ghost"
              size="icon"
              onClick={handleSignOut}
              className="hover:text-destructive"
              title="Cerrar sesión"
            >
              <LogOut className="size-5" />
            </Button>
        </div>
      </CardHeader>
      <CardContent className="space-y-4">
        <Alert variant="default" className="bg-background border-primary/20">
          <Info className="h-4 w-4 text-primary" />
          <AlertTitle className="text-xs font-bold uppercase tracking-wider text-primary">Paso a paso</AlertTitle>
          <AlertDescription className="text-xs text-muted-foreground">
            Copia este UID y pégalo en el código Arduino. No olvides crear el usuario "esp32@safeday.com" con la contraseña que elijas en el panel de Firebase.
          </AlertDescription>
        </Alert>

        <div className="space-y-2">
            <span className="text-[10px] font-bold uppercase text-muted-foreground">Tu Device UID:</span>
            {isLoading ? (
            <Skeleton className="h-10 w-full" />
            ) : user ? (
            <div className="flex items-center gap-2">
                <pre className="truncate rounded-md bg-background px-3 py-2 text-xs font-mono text-muted-foreground w-full border border-primary/20">
                <code>{user.uid}</code>
                </pre>
                <Button
                variant="outline"
                size="icon"
                onClick={handleCopy}
                className="shrink-0 border-primary/20 hover:bg-primary/10"
                >
                {copied ? (
                    <Check className="size-4 text-primary" />
                ) : (
                    <Copy className="size-4" />
                )}
                </Button>
            </div>
            ) : (
            <p className="text-sm text-destructive">
                Inicia sesión para ver tu UID.
            </p>
            )}
        </div>
      </CardContent>
    </Card>
  );
}
