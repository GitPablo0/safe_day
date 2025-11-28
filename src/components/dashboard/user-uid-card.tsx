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
import { KeyRound, Copy, Check, LogOut } from 'lucide-react';
import { useAuth } from '@/firebase';

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
      setTimeout(() => setCopied(false), 2000); // Reset after 2 seconds
    }
  };

  const handleSignOut = async () => {
    await signOut(auth);
    // The redirect will be handled by the dashboard component
  };

  return (
    <Card>
      <CardHeader>
        <div className="flex justify-between items-start">
            <div>
                <CardTitle className="flex items-center gap-2">
                <KeyRound className="size-5" />
                <span>Device UID</span>
                </CardTitle>
                <CardDescription>
                Usa este UID para tu ESP32.
                </CardDescription>
            </div>
             <Button
              variant="ghost"
              size="icon"
              onClick={handleSignOut}
              aria-label="Cerrar sesión"
              title="Cerrar sesión"
            >
              <LogOut className="size-5" />
            </Button>
        </div>
      </CardHeader>
      <CardContent>
        {isLoading ? (
          <Skeleton className="h-10 w-full" />
        ) : user ? (
          <div className="flex items-center gap-2">
            <pre className="truncate rounded-md bg-muted px-3 py-2 text-sm text-muted-foreground w-full">
              <code>{user.uid}</code>
            </pre>
            <Button
              variant="outline"
              size="icon"
              onClick={handleCopy}
              aria-label="Copiar UID"
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
            No se pudo obtener el UID.
          </p>
        )}
      </CardContent>
    </Card>
  );
}
