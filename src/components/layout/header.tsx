import { BrainCircuit } from 'lucide-react';
import Link from 'next/link';

export default function Header() {
  return (
    <header className="border-b">
      <div className="container mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex h-16 items-center justify-between">
          <Link href="/" className="flex items-center gap-2">
            <BrainCircuit className="h-7 w-7 text-primary" />
            <span className="text-xl font-bold text-foreground">
              NeuroSense
            </span>
          </Link>
          {/* Future navigation items can be added here */}
        </div>
      </div>
    </header>
  );
}
