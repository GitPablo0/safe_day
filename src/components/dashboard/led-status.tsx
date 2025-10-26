"use client";

import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { cn } from "@/lib/utils";

interface LedStatusProps {
  level: 1 | 2 | 3;
}

const statusConfig = {
  1: {
    label: "Calm",
    color: "bg-green-500",
    description: "Environment is stable."
  },
  2: {
    label: "Elevated",
    color: "bg-orange-400",
    description: "Moderate environmental stimuli detected."
  },
  3: {
    label: "High Stress",
    color: "bg-red-500",
    description: "High environmental stimuli detected."
  },
};

export default function LedStatus({ level }: LedStatusProps) {
  const currentStatus = statusConfig[level];

  return (
    <Card className="flex flex-col">
      <CardHeader>
        <CardTitle>System Status</CardTitle>
        <p className="text-sm text-muted-foreground">{currentStatus.description}</p>
      </CardHeader>
      <CardContent className="flex-grow flex flex-col items-center justify-center gap-4">
        <div className="flex items-center space-x-4">
          <div
            className={cn(
              "h-8 w-8 rounded-full transition-all duration-300",
              level === 1 ? "bg-green-500 scale-110 shadow-lg shadow-green-500/50" : "bg-muted scale-90"
            )}
          />
          <div
            className={cn(
              "h-8 w-8 rounded-full transition-all duration-300",
              level === 2 ? "bg-orange-400 scale-110 shadow-lg shadow-orange-400/50" : "bg-muted scale-90"
            )}
          />
          <div
            className={cn(
              "h-8 w-8 rounded-full transition-all duration-300",
              level === 3 ? "bg-red-500 scale-110 shadow-lg shadow-red-500/50 animate-pulse" : "bg-muted scale-90"
            )}
          />
        </div>
        <p className="text-2xl font-bold mt-4" style={{color: `var(--${currentStatus.color.replace('bg-', '').replace('-500', '').replace('-400', '')})`}}>
          {currentStatus.label}
        </p>
      </CardContent>
    </Card>
  );
}
