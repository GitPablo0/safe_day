'use client';

import Link from 'next/link';
import { usePathname, useRouter } from 'next/navigation';
import { Shield, LayoutDashboard, Settings, LogOut } from 'lucide-react';
import { cn } from '@/lib/utils';
import { Button } from '@/components/ui/button';
import { useAuth } from '@/firebase';
import { signOut } from 'firebase/auth';

const navItems = [
  { href: '/panel', label: 'Panel de Control', icon: LayoutDashboard },
  { href: '/configuracion', label: 'Configuración', icon: Settings },
];

export default function SidebarNav() {
  const pathname = usePathname();
  const router = useRouter();
  const auth = useAuth();

  const handleSignOut = async () => {
    if (auth) {
      await signOut(auth);
      router.push('/login');
    }
  };

  return (
    <aside className="w-64 flex-shrink-0 border-r border-border bg-card p-4 flex flex-col">
      <div className="flex items-center gap-3 px-2 py-4">
        <Shield className="h-8 w-8 text-primary" />
        <span className="text-xl font-bold">SafePlace</span>
      </div>
      <nav className="flex-grow mt-8 space-y-2">
        {navItems.map((item) => (
          <Link
            key={item.href}
            href={item.href}
            className={cn(
              'flex items-center gap-3 rounded-lg px-3 py-2 text-muted-foreground transition-all hover:text-primary',
              pathname === item.href && 'bg-muted text-primary'
            )}
          >
            <item.icon className="h-5 w-5" />
            {item.label}
          </Link>
        ))}
      </nav>
      <div className="mt-auto">
        <Button variant="ghost" className="w-full justify-start gap-3" onClick={handleSignOut}>
          <LogOut className="h-5 w-5" />
          Cerrar Sesión
        </Button>
      </div>
    </aside>
  );
}
