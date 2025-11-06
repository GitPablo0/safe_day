
/**
 * ==================================================================================
 * Este es un archivo de ejemplo para el funcionamiento autónomo del proyecto NeuroSense
 * en un microcontrolador (como un ESP32), sin necesidad de conexión a Firebase o WiFi.
 *
 * --- INSTRUCCIONES DE CONFIGURACIÓN MANUAL ---
 * Modifica los valores en la sección "4. UMBRALES Y LÓGICA DE EVENTOS" para ajustar
 * la sensibilidad de los sensores y el comportamiento de los LEDs.
 * ==================================================================================
 */
#include <Arduino.h>
#include "DHT.h"

// ===============================================
// 1. PINES Y SENSORES
// ===============================================
#define DHTPIN 4
#define DHTTYPE DHT22
const int SOUND_SENSOR_PIN = 34;
const int LIGHT_SENSOR_PIN = 35; // Pin para el sensor de luz

DHT dht(DHTPIN, DHTTYPE);

const int LED_VERDE_PIN = 32;
const int LED_NARANJA_PIN = 33;
const int LED_ROJO_PIN = 25;

// ===============================================
// 2. UMBRALES Y LÓGICA DE EVENTOS (CONFIGURACIÓN MANUAL)
// ===============================================
// Sonido
float soundThresholdLow = 300;
float soundThresholdMedium = 380;
int orangeSoundEventCountThreshold = 3;
int redSoundEventCountThreshold = 3;
long timeBetweenEventsMillis = 10000;
long orangeBlockDurationMillis = 60000;
long redBlockDurationMillis = 60000;

// Luz
float lightThresholdLow = 400;
float lightThresholdMedium = 600;
int orangeLightEventCountThreshold = 3;
int redLightEventCountThreshold = 3;
long timeBetweenLightEventsMillis = 10000;
long orangeLightBlockDurationMillis = 60000;
long redLightBlockDurationMillis = 60000;

// Temperatura
float tempThresholdVerde = 24;
float tempThresholdNaranja = 28;

// ===============================================
// 3. VARIABLES DE ESTADO
// ===============================================
unsigned long sendDataPrevMillis = 0;
const long sendDataInterval = 2000; // Intervalo de lectura aumentado a 2 segundos para no saturar el monitor

// Variables para eventos de sonido
const int MAX_SOUND_EVENTS = 10;
long orangeEventTimestamps[MAX_SOUND_EVENTS];
int orangeEventIndex = 0;
long redEventTimestamps[MAX_SOUND_EVENTS];
int redEventIndex = 0;

// Variables para eventos de luz
const int MAX_LIGHT_EVENTS = 10;
long orangeLightEventTimestamps[MAX_LIGHT_EVENTS];
int orangeLightEventIndex = 0;
long redLightEventTimestamps[MAX_LIGHT_EVENTS];
int redLightEventIndex = 0;

// Variables de bloqueo de LEDs
unsigned long orangeLedBlockedUntil = 0;
unsigned long redLedBlockedUntil = 0;
unsigned long orangeLightLedBlockedUntil = 0;
unsigned long redLightLedBlockedUntil = 0;

int currentLedStatus = 1;

void setup() {
    Serial.begin(115200);
    Serial.println("Inicializando NeuroSense en modo autónomo...");

    dht.begin();
    delay(2000);

    pinMode(LED_VERDE_PIN, OUTPUT);
    pinMode(LED_NARANJA_PIN, OUTPUT);
    pinMode(LED_ROJO_PIN, OUTPUT);
    digitalWrite(LED_VERDE_PIN, LOW);
    digitalWrite(LED_NARANJA_PIN, LOW);
    digitalWrite(LED_ROJO_PIN, LOW);

    // Inicializar arrays de timestamps a cero
    for (int i = 0; i < MAX_SOUND_EVENTS; i++) {
        orangeEventTimestamps[i] = 0;
        redEventTimestamps[i] = 0;
    }
    for (int i = 0; i < MAX_LIGHT_EVENTS; i++) {
        orangeLightEventTimestamps[i] = 0;
        redLightEventTimestamps[i] = 0;
    }

    Serial.println("Configuración manual cargada. El dispositivo está listo.");
    digitalWrite(LED_VERDE_PIN, HIGH); // Encender LED verde al inicio
}

float readTemperature() {
    float t = dht.readTemperature();
    return isnan(t) ? 0 : t;
}

float readHumidity() {
    float h = dht.readHumidity();
    return isnan(h) ? 0 : h;
}

int readSoundLevel() {
    const int sampleWindow = 50;
    unsigned int peakToPeak = 0;
    unsigned int signalMax = 0;
    unsigned int signalMin = 4095;
    unsigned long startMillis = millis();
    while (millis() - startMillis < sampleWindow) {
        unsigned int sample = analogRead(SOUND_SENSOR_PIN);
        if (sample < 4095) {
            if (sample > signalMax) signalMax = sample;
            else if (sample < signalMin) signalMin = sample;
        }
    }
    peakToPeak = signalMax - signalMin;
    return peakToPeak;
}

int readLightLevel() {
    return analogRead(LIGHT_SENSOR_PIN);
}

int countRecentEvents(long timestamps[], long windowMillis, int maxEvents) {
    int count = 0;
    unsigned long now = millis();
    for (int i = 0; i < maxEvents; i++) {
        if (timestamps[i] > 0 && (now - timestamps[i]) < windowMillis) {
            count++;
        }
    }
    return count;
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - sendDataPrevMillis >= sendDataInterval) {
        sendDataPrevMillis = currentMillis;

        float temperature = readTemperature();
        int soundLevel = readSoundLevel();
        int lightLevel = readLightLevel();
        
        // --- Lógica de Eventos de Sonido ---
        bool orangeSoundStateActive = orangeLedBlockedUntil > currentMillis;
        if (soundLevel >= soundThresholdLow) {
            orangeEventTimestamps[orangeEventIndex] = currentMillis;
            orangeEventIndex = (orangeEventIndex + 1) % MAX_SOUND_EVENTS;
            if (!orangeSoundStateActive && countRecentEvents(orangeEventTimestamps, timeBetweenEventsMillis, MAX_SOUND_EVENTS) >= orangeSoundEventCountThreshold) {
                orangeLedBlockedUntil = currentMillis + orangeBlockDurationMillis;
            }
        }
        
        bool redSoundStateActive = redLedBlockedUntil > currentMillis;
        if (orangeSoundStateActive && soundLevel >= soundThresholdMedium) {
            redEventTimestamps[redEventIndex] = currentMillis;
            redEventIndex = (redEventIndex + 1) % MAX_SOUND_EVENTS;
            if (!redSoundStateActive && countRecentEvents(redEventTimestamps, timeBetweenEventsMillis, MAX_SOUND_EVENTS) >= redSoundEventCountThreshold) {
                redLedBlockedUntil = currentMillis + redBlockDurationMillis;
                for(int i=0; i<MAX_SOUND_EVENTS; i++) { orangeEventTimestamps[i] = 0; } // Reiniciar eventos naranja
                orangeEventIndex = 0;
            }
        }
        
        // --- Lógica de Eventos de Luz ---
        bool orangeLightStateActive = orangeLightLedBlockedUntil > currentMillis;
        if (lightLevel >= lightThresholdLow) {
            orangeLightEventTimestamps[orangeLightEventIndex] = currentMillis;
            orangeLightEventIndex = (orangeLightEventIndex + 1) % MAX_LIGHT_EVENTS;
            if (!orangeLightStateActive && countRecentEvents(orangeLightEventTimestamps, timeBetweenLightEventsMillis, MAX_LIGHT_EVENTS) >= orangeLightEventCountThreshold) {
                orangeLightLedBlockedUntil = currentMillis + orangeLightBlockDurationMillis;
            }
        }

        bool redLightStateActive = redLightLedBlockedUntil > currentMillis;
        if (orangeLightStateActive && lightLevel >= lightThresholdMedium) {
            redLightEventTimestamps[redLightEventIndex] = currentMillis;
            redLightEventIndex = (redLightEventIndex + 1) % MAX_LIGHT_EVENTS;
            if (!redLightStateActive && countRecentEvents(redLightEventTimestamps, timeBetweenLightEventsMillis, MAX_LIGHT_EVENTS) >= redLightEventCountThreshold) {
                redLightLedBlockedUntil = currentMillis + redLightBlockDurationMillis;
                for(int i=0; i<MAX_LIGHT_EVENTS; i++) { orangeLightEventTimestamps[i] = 0; } // Reiniciar eventos de luz naranja
                orangeLightEventIndex = 0;
            }
        }
        
        // --- Lógica de Determinación de Estado del LED ---
        int estadoSonidoBloqueado = 1;
        if (orangeLedBlockedUntil > currentMillis) estadoSonidoBloqueado = 2;
        if (redLedBlockedUntil > currentMillis) estadoSonidoBloqueado = 3;

        int estadoLuzBloqueado = 1;
        if (orangeLightLedBlockedUntil > currentMillis) estadoLuzBloqueado = 2;
        if (redLightLedBlockedUntil > currentMillis) estadoLuzBloqueado = 3;
        
        int estadoSonidoInstantaneo = 1;
        if (soundLevel >= soundThresholdLow) estadoSonidoInstantaneo = 2;
        if (soundLevel >= soundThresholdMedium) estadoSonidoInstantaneo = 3;

        int estadoLuzInstantaneo = 1;
        if (lightLevel >= lightThresholdLow) estadoLuzInstantaneo = 2;
        if (lightLevel >= lightThresholdMedium) estadoLuzInstantaneo = 3;

        int estadoTemp = 1;
        if (temperature >= tempThresholdVerde) estadoTemp = 2;
        if (temperature >= tempThresholdNaranja) estadoTemp = 3;

        // El estado final es el máximo de todos los estados calculados
        currentLedStatus = 1;
        currentLedStatus = max(currentLedStatus, estadoTemp);
        currentLedStatus = max(currentLedStatus, estadoSonidoInstantaneo);
        currentLedStatus = max(currentLedStatus, estadoLuzInstantaneo);
        currentLedStatus = max(currentLedStatus, estadoSonidoBloqueado);
        currentLedStatus = max(currentLedStatus, estadoLuzBloqueado);

        digitalWrite(LED_VERDE_PIN, currentLedStatus == 1);
        digitalWrite(LED_NARANJA_PIN, currentLedStatus == 2);
        digitalWrite(LED_ROJO_PIN, currentLedStatus == 3);

        // --- Salida al Monitor Serie ---
        Serial.print("Temp: ");
        Serial.print(temperature);
        Serial.print(" *C, Sonido: ");
        Serial.print(soundLevel);
        Serial.print(", Luz: ");
        Serial.print(lightLevel);
        Serial.print(" -> Estado LED: ");
        Serial.println(currentLedStatus);
    }
}
