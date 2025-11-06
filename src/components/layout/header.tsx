'use client';

import { useState } from 'react';
import Link from 'next/link';
import { useRouter } from 'next/navigation';
import { Shield, Menu, X, LayoutDashboard, Settings, LogOut } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Sheet, SheetContent, SheetTitle, SheetTrigger } from '@/components/ui/sheet';
import { useAuth } from '@/firebase';
import { signOut } from 'firebase/auth';
import { cn } from '@/lib/utils';

const navItems = [
  { href: '/panel', label: 'Panel de Control', icon: LayoutDashboard },
  { href: '/configuracion', label: 'Configuración', icon: Settings },
];

export default function Header() {
  const router = useRouter();
  const auth = useAuth();
  const [isSheetOpen, setIsSheetOpen] = useState(false);

  const handleSignOut = async () => {
    if (auth) {
      await signOut(auth);
      router.push('/login');
    }
  };

  const closeSheet = () => setIsSheetOpen(false);

  return (
    <header className="bg-card border-b border-border sticky top-0 z-50">
      <div className="container mx-auto flex items-center justify-between h-16 px-4">
        <Link href="/panel" className="flex items-center gap-3">
          <Shield className="h-8 w-8 text-primary" />
          <span className="text-xl font-bold hidden sm:inline">SafePlace</span>
        </Link>
        
        <Sheet open={isSheetOpen} onOpenChange={setIsSheetOpen}>
          <SheetTrigger asChild>
            <Button variant="ghost" size="icon">
              <Menu className="h-6 w-6" />
              <span className="sr-only">Abrir menú</span>
            </Button>
          </SheetTrigger>
          <SheetContent side="right" className="w-[300px] flex flex-col p-0">
            <SheetTitle className="sr-only">Menú de Navegación</SheetTitle>
            <div className="flex items-center justify-between p-4 border-b border-border">
                <div className="flex items-center gap-3">
                    <Shield className="h-8 w-8 text-primary" />
                    <span className="text-xl font-bold">SafePlace</span>
                </div>
                <Button variant="ghost" size="icon" onClick={closeSheet} className="text-destructive hover:bg-destructive/10">
                    <X className="h-6 w-6" />
                    <span className="sr-only">Cerrar menú</span>
                </Button>
            </div>
            
            <nav className="flex-grow p-4 space-y-2">
              {navItems.map((item) => (
                <Link
                  key={item.href}
                  href={item.href}
                  onClick={closeSheet}
                  className={cn(
                    'flex items-center gap-3 rounded-lg px-3 py-3 text-lg text-muted-foreground transition-all hover:bg-muted hover:text-primary',
                  )}
                >
                  <item.icon className="h-6 w-6" />
                  {item.label}
                </Link>
              ))}
            </nav>

            <div className="p-4 border-t border-border">
              <Button variant="ghost" className="w-full justify-start gap-3 text-lg" onClick={handleSignOut}>
                <LogOut className="h-6 w-6" />
                Cerrar Sesión
              </Button>
            </div>
          </SheetContent>
        </Sheet>
      </div>
    </header>
  );
}
