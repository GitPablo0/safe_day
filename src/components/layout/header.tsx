'use client';

import { Shield, LogOut } from 'lucide-react';
import Link from 'next/link';
import { useAuth, useUser } from '@/firebase';
import { getAuth, signOut } from 'firebase/auth';
import { Button } from '../ui/button';
import { useRouter } from 'next/navigation';

export default function Header() {
  const { user } = useUser();
  const auth = useAuth();
  const router = useRouter();

  const handleSignOut = async () => {
    if (auth) {
      await signOut(auth);
      router.push('/login');
    }
  };

  return (
    <header className="border-b">
      <div className="container mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex h-16 items-center justify-between">
          <Link href="/" className="flex items-center gap-2">
            <Shield className="h-7 w-7 text-primary" />
            <span className="text-xl font-bold text-foreground">
              SafePlace
            </span>
          </Link>
          {user && (
            <Button variant="ghost" size="icon" onClick={handleSignOut}>
              <LogOut className="h-5 w-5" />
              <span className="sr-only">Cerrar Sesión</span>
            </Button>
          )}
        </div>
      </div>
    </header>
  );
}
