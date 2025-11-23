"use client";

import { useState, useEffect } from "react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { cn } from "@/lib/utils";

interface LedStatusProps {
  level: 1 | 2 | 3;
  timeRemaining: number;
}

const statusConfig = {
  1: {
    label: "Calma",
    color: "bg-green-500",
    textColor: "text-green-500",
    description: "El ambiente está estable."
  },
  2: {
    label: "Elevado",
    color: "bg-orange-400",
    textColor: "text-orange-400",
    description: "Se detectaron estímulos moderados."
  },
  3: {
    label: "Estrés Alto",
    color: "bg-red-500",
    textColor: "text-red-500",
    description: "Se detectaron estímulos altos."
  },
};

export default function LedStatus({ level, timeRemaining }: LedStatusProps) {
  const [countdown, setCountdown] = useState(Math.ceil(timeRemaining / 1000));
  const currentStatus = statusConfig[level];

  useEffect(() => {
    setCountdown(Math.ceil(timeRemaining / 1000));

    if (timeRemaining > 0) {
      const interval = setInterval(() => {
        setCountdown(prev => {
          if (prev <= 1) {
            clearInterval(interval);
            return 0;
          }
          return prev - 1;
        });
      }, 1000);

      return () => clearInterval(interval);
    }
  }, [timeRemaining]);


  return (
    <Card className="flex flex-col h-full">
      <CardHeader>
        <CardTitle>Estado del Sistema</CardTitle>
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
        <p className={cn("text-2xl font-bold mt-4", currentStatus.textColor)}>
          {currentStatus.label}
        </p>
        {countdown > 0 && level > 1 && (
            <div className="text-center mt-2">
                <p className="text-xs text-muted-foreground">Volviendo a tiempo real en</p>
                <p className="text-lg font-semibold">{countdown}s</p>
            </div>
        )}
      </CardContent>
    </Card>
  );
}
