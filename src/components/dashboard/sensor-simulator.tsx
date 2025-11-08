"use client";

import { useState } from 'react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Zap } from "lucide-react";
import { useToast } from "@/hooks/use-toast";

interface SensorSimulatorProps {
    onSendSampleData: (data: { temperature: number; humidity: number; soundLevel: number; lightLevel: number }) => void;
}

export default function SensorSimulator({ onSendSampleData }: SensorSimulatorProps) {
    const { toast } = useToast();

    const generateRandomData = () => {
        const temperature = parseFloat((Math.random() * 15 + 15).toFixed(1)); // 15.0 to 30.0
        const humidity = parseFloat((Math.random() * 50 + 30).toFixed(1)); // 30.0 to 80.0
        const soundLevel = Math.floor(Math.random() * 800); // 0 to 800
        const lightLevel = Math.floor(Math.random() * 1000); // 0 to 1000
        return { temperature, humidity, soundLevel, lightLevel };
    }

    const handleSend = () => {
        const data = generateRandomData();
        onSendSampleData(data);
    };

    return (
        <Card className="h-full">
            <CardHeader>
                <CardTitle>Simulador de Sensor</CardTitle>
                <CardDescription>
                    Envía datos de sensor aleatorios para probar la aplicación sin hardware.
                </CardDescription>
            </CardHeader>
            <CardContent className="flex items-center justify-center h-full pb-16">
                <Button onClick={handleSend}>
                    <Zap className="mr-2 h-4 w-4" />
                    Enviar Datos de Muestra
                </Button>
            </CardContent>
        </Card>
    );
}
