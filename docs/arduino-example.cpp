/**
 * ==================================================================================
 * Proyecto NeuroSense - Firmware para ESP32
 *
 * Este código conecta el dispositivo a WiFi y Firebase para sincronizar datos
 * de sensores y configuraciones en tiempo real con la aplicación web.
 *
 * --- INSTRUCCIONES DE CONFIGURACIÓN ---
 * 1. Instala la librería "Firebase ESP Client" de Mobizt desde el gestor de librerías.
 * 2. Rellena tus credenciales en la sección "CONFIGURACIÓN DE WIFI Y FIREBASE".
 * ==================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "DHT.h"
#include <HardwareSerial.h>
#include "DFRobotDFPlayerMini.h"

// ===============================================
// 1. CONFIGURACIÓN DE WIFI Y FIREBASE (DEBES RELLENAR ESTO)
// ===============================================
#define WIFI_SSID "Telecentro-416b"
#define WIFI_PASSWORD "ATNMFMY2MM5Q"

// Pega aquí la API Key y el Project ID de tu proyecto de Firebase
#define API_KEY "AIzaSyCMmir0FR57YVoVGQ9QCUG4oG7bUEF3w7I"
#define FIREBASE_PROJECT_ID "studio-4253330981-dcdc9"

// Pega aquí el User ID que aparece en el panel de la aplicación web
#define USER_UID "eOp4B72C0wdVqf6PZDOOQGGYRdP2"


// ===============================================
// 2. PINES Y SENSORES
// ===============================================
#define DHTPIN 4
#define DHTTYPE DHT22
const int SOUND_SENSOR_PIN = 34;
const int LIGHT_SENSOR_PIN = 36;

DHT dht(DHTPIN, DHTTYPE);

const int LED_VERDE_PIN = 32;
const int LED_NARANJA_PIN = 33;
const int LED_ROJO_PIN = 25;

// ===============================================
// 3. CONFIGURACIÓN DFPLAYER MINI
// ===============================================
const int DFPLAYER_RX_PIN = 26; // Conectar al TX del DFPlayer
const int DFPLAYER_TX_PIN = 27; // Conectar al RX del DFPlayer

HardwareSerial mySoftwareSerial(1);
DFRobotDFPlayerMini myDFPlayer;
bool isPlaying = false;


// ===============================================
// 4. UMBRALES Y LÓGICA DE EVENTOS (Valores por defecto)
// Estos valores serán sobreescritos por la configuración de Firebase si está disponible.
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
float lightThresholdLow = 3000;
float lightThresholdMedium = 1000;
int orangeLightEventCountThreshold = 3;
int redLightEventCountThreshold = 3;
long timeBetweenLightEventsMillis = 10000;
long orangeLightBlockDurationMillis = 60000;
long redLightBlockDurationMillis = 60000;

// Temperatura
float tempThresholdVerde = 24;
float tempThresholdNaranja = 28;

// Música
const unsigned long musicPlayDuration = 180000; // 3 minutos

// ===============================================
// 5. VARIABLES DE ESTADO Y FIREBASE
// ===============================================
unsigned long sendDataPrevMillis = 0;
const long sendDataInterval = 1000; // Intervalo de lectura de sensores
const long firebaseUpdateInterval = 1000; // Intervalo de envío de datos a Firebase

FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

String thresholdConfigPath;
String sensorDataPath;
bool firebaseReady = false;

// Variables para eventos
const int MAX_EVENTS = 10;
long orangeSoundEventTimestamps[MAX_EVENTS];
int orangeSoundEventIndex = 0;
long redSoundEventTimestamps[MAX_EVENTS];
int redSoundEventIndex = 0;
long orangeLightEventTimestamps[MAX_EVENTS];
int orangeLightEventIndex = 0;
long redLightEventTimestamps[MAX_EVENTS];
int redLightEventIndex = 0;

unsigned long orangeLedBlockedUntil = 0;
unsigned long redLedBlockedUntil = 0;
unsigned long orangeLightLedBlockedUntil = 0;
unsigned long redLightLedBlockedUntil = 0;

int currentLedStatus = 1;
unsigned long musicStopTime = 0;

// ===============================================
// 6. FUNCIONES AUXILIARES DE FIREBASE
// ===============================================

// Función para procesar un objeto JSON de configuración
void processConfigJson(FirebaseJson* json) {
    FirebaseJsonData jsonData;
    
    if (json->get(jsonData, "soundThresholdLow")) soundThresholdLow = jsonData.to<float>();
    if (json->get(jsonData, "soundThresholdMedium")) soundThresholdMedium = jsonData.to<float>();
    if (json->get(jsonData, "lightThresholdLow")) lightThresholdLow = jsonData.to<float>();
    if (json->get(jsonData, "lightThresholdMedium")) lightThresholdMedium = jsonData.to<float>();
    if (json->get(jsonData, "temperatureThresholdVerde")) tempThresholdVerde = jsonData.to<float>();
    if (json->get(jsonData, "temperatureThresholdNaranja")) tempThresholdNaranja = jsonData.to<float>();
    
    if (json->get(jsonData, "orangeSoundEventCount")) orangeSoundEventCountThreshold = jsonData.to<int>();
    if (json->get(jsonData, "redSoundEventCount")) redSoundEventCountThreshold = jsonData.to<int>();
    if (json->get(jsonData, "timeBetweenEvents")) timeBetweenEventsMillis = jsonData.to<long>();
    if (json->get(jsonData, "orangeBlockDuration")) orangeBlockDurationMillis = jsonData.to<long>();
    if (json->get(jsonData, "redBlockDuration")) redBlockDurationMillis = jsonData.to<long>();
    
    if (json->get(jsonData, "orangeLightEventCount")) orangeLightEventCountThreshold = jsonData.to<int>();
    if (json->get(jsonData, "redLightEventCount")) redLightEventCountThreshold = jsonData.to<int>();
    if (json->get(jsonData, "timeBetweenLightEvents")) timeBetweenLightEventsMillis = jsonData.to<long>();
    if (json->get(jsonData, "orangeLightBlockDuration")) orangeLightBlockDurationMillis = jsonData.to<long>();
    if (json->get(jsonData, "redLightBlockDuration")) redLightBlockDurationMillis = jsonData.to<long>();

    Serial.println("Umbrales actualizados desde Firebase.");
}


// Callback que se ejecuta cuando hay un cambio en la configuración de umbrales
void streamCallback(FirebaseStream data) {
  Serial.println("Stream de configuración recibido. Actualizando umbrales...");
  if (data.dataType() == "json") {
    FirebaseJson* json = data.to<FirebaseJson*>();
    processConfigJson(json);
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Stream de configuración timed out. Reiniciando stream...");
  }
}

void setupFirebase() {
    config.api_key = API_KEY;
    config.database_url = "studio-4253330981-dcdc9-default-rtdb.firebaseio.com";
    auth.user.email = ""; // Usamos Custom Token, no es necesario email/pass
    auth.user.password = "";
    
    config.signer.test_mode = true;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    
    Serial.print("Conectando a Firebase...");
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      Serial.print(".");
      delay(300);
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Conectado a WiFi.");
        thresholdConfigPath = "users/" + String(USER_UID) + "/thresholdConfigurations/main";
        sensorDataPath = "users/" + String(USER_UID) + "/sensorData/latest";

        if (Firebase.RTDB.getJSON(&fbdo, thresholdConfigPath)) {
            if(fbdo.dataType() == "json"){
                FirebaseJson* json = fbdo.to<FirebaseJson*>();
                processConfigJson(json);
            } else {
                Serial.println("No se pudo obtener la configuración inicial. Usando valores por defecto.");
                Serial.println(fbdo.errorReason());
            }
        } else {
            Serial.println("No se pudo obtener la configuración inicial. Usando valores por defecto.");
            Serial.println(fbdo.errorReason());
        }
        
        if (!Firebase.RTDB.beginStream(&stream, thresholdConfigPath)) {
            Serial.println("No se pudo iniciar el stream de configuración.");
            Serial.println(stream.errorReason());
        } else {
            Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);
            firebaseReady = true;
            Serial.println("Firebase listo y escuchando cambios.");
        }
    } else {
        Serial.println("Error al conectar a WiFi.");
    }
}


// ===============================================
// 7. FUNCIONES DE LECTURA DE SENSORES
// ===============================================
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
    long total = 0;
    int samples = 10;
    for (int i = 0; i < samples; i++) {
        total += analogRead(LIGHT_SENSOR_PIN);
        delay(5);
    }
    return total / samples;
}

// ===============================================
// 8. LÓGICA PRINCIPAL Y SETUP/LOOP
// ===============================================
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

void printStateReason(int finalStatus, int temp, int soundI, int lightI, int soundB, int lightB) {
    if (finalStatus == 1) {
        Serial.println(" -> Causa: Calma");
        return;
    }
    
    String reason = " -> Causa: ";
    if (finalStatus == lightB) reason += (finalStatus == 3 ? "Bloqueo Rojo por Luz" : "Bloqueo Naranja por Luz");
    else if (finalStatus == soundB) reason += (finalStatus == 3 ? "Bloqueo Rojo por Sonido" : "Bloqueo Naranja por Sonido");
    else if (finalStatus == lightI) reason += "Luz Instantánea";
    else if (finalStatus == soundI) reason += "Sonido Instantáneo";
    else if (finalStatus == temp) reason += "Temperatura";
    else reason += "Desconocida";
    Serial.println(reason);
}


void setup() {
    Serial.begin(115200);
    Serial.println("Inicializando NeuroSense...");

    mySoftwareSerial.begin(9600, SERIAL_8N1, DFPLAYER_TX_PIN, DFPLAYER_RX_PIN);
    dht.begin();
    delay(2000);

    pinMode(LED_VERDE_PIN, OUTPUT);
    pinMode(LED_NARANJA_PIN, OUTPUT);
    pinMode(LED_ROJO_PIN, OUTPUT);
    digitalWrite(LED_VERDE_PIN, LOW);
    digitalWrite(LED_NARANJA_PIN, LOW);
    digitalWrite(LED_ROJO_PIN, LOW);

    // Conectar a WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Conectando a WiFi ");
    int wifi_retries = 20;
    while (WiFi.status() != WL_CONNECTED && wifi_retries > 0) {
        Serial.print(".");
        delay(500);
        wifi_retries--;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Conectado! IP: ");
        Serial.println(WiFi.localIP());
        setupFirebase();
    } else {
        Serial.println("No se pudo conectar a WiFi. El dispositivo funcionará en modo offline.");
    }
    
    Serial.println("Inicializando DFPlayer...");
    if (!myDFPlayer.begin(mySoftwareSerial, false)) { // 'false' para no esperar respuesta, útil si hay ruido al inicio
        Serial.println("No se pudo iniciar el DFPlayer. Verifica conexiones.");
        // No se detiene, para que el resto del dispositivo siga funcionando
    } else {
       Serial.println("DFPlayer Mini iniciado.");
       myDFPlayer.volume(15);
    }

    Serial.println("Configuración finalizada. El dispositivo está listo.");
    digitalWrite(LED_VERDE_PIN, HIGH);
}


void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - sendDataPrevMillis >= sendDataInterval) {
        sendDataPrevMillis = currentMillis;

        float temperature = readTemperature();
        float humidity = readHumidity();
        int soundLevel = readSoundLevel();
        int lightLevel = readLightLevel();
        
        // --- Lógica de Eventos de Sonido ---
        if (soundLevel >= soundThresholdLow) {
            orangeSoundEventTimestamps[orangeSoundEventIndex] = currentMillis;
            orangeSoundEventIndex = (orangeSoundEventIndex + 1) % MAX_EVENTS;
            if (orangeLedBlockedUntil <= currentMillis && countRecentEvents(orangeSoundEventTimestamps, timeBetweenEventsMillis, MAX_EVENTS) >= orangeSoundEventCountThreshold) {
                orangeLedBlockedUntil = currentMillis + orangeBlockDurationMillis;
            }
        }
        
        if (soundLevel >= soundThresholdMedium) {
            redSoundEventTimestamps[redSoundEventIndex] = currentMillis;
            redSoundEventIndex = (redSoundEventIndex + 1) % MAX_EVENTS;
            if (redLedBlockedUntil <= currentMillis && countRecentEvents(redSoundEventTimestamps, timeBetweenEventsMillis, MAX_EVENTS) >= redSoundEventCountThreshold) {
                redLedBlockedUntil = currentMillis + redBlockDurationMillis;
                for(int i=0; i<MAX_EVENTS; i++) { orangeSoundEventTimestamps[i] = 0; } 
                orangeEventIndex = 0;
            }
        }
        
        // --- Lógica de Eventos de Luz ---
        if (lightLevel <= lightThresholdLow) {
            orangeLightEventTimestamps[orangeLightEventIndex] = currentMillis;
            orangeLightEventIndex = (orangeLightEventIndex + 1) % MAX_EVENTS;
            if (orangeLightLedBlockedUntil <= currentMillis && countRecentEvents(orangeLightEventTimestamps, timeBetweenLightEventsMillis, MAX_EVENTS) >= orangeLightEventCountThreshold) {
                orangeLightLedBlockedUntil = currentMillis + orangeLightBlockDurationMillis;
            }
        }

        if (lightLevel <= lightThresholdMedium) {
            redLightEventTimestamps[redLightEventIndex] = currentMillis;
            redLightEventIndex = (redLightEventIndex + 1) % MAX_EVENTS;
            if (redLightLedBlockedUntil <= currentMillis && countRecentEvents(redLightEventTimestamps, timeBetweenLightEventsMillis, MAX_EVENTS) >= redLightEventCountThreshold) {
                redLightLedBlockedUntil = currentMillis + redLightBlockDurationMillis;
                for(int i=0; i<MAX_EVENTS; i++) { orangeLightEventTimestamps[i] = 0; }
                orangeLightEventIndex = 0;
            }
        }
        
        // --- Lógica de Determinación de Estado del LED ---
        int estadoSonidoBloqueado = (redLedBlockedUntil > currentMillis) ? 3 : ((orangeLedBlockedUntil > currentMillis) ? 2 : 1);
        int estadoLuzBloqueado = (redLightLedBlockedUntil > currentMillis) ? 3 : ((orangeLightLedBlockedUntil > currentMillis) ? 2 : 1);
        int estadoSonidoInstantaneo = (soundLevel >= soundThresholdMedium) ? 3 : ((soundLevel >= soundThresholdLow) ? 2 : 1);
        int estadoLuzInstantaneo = (lightLevel <= lightThresholdMedium) ? 3 : ((lightLevel <= lightThresholdLow) ? 2 : 1);
        int estadoTemp = (temperature >= tempThresholdNaranja) ? 3 : ((temperature >= tempThresholdVerde) ? 2 : 1);

        currentLedStatus = max({estadoTemp, estadoSonidoInstantaneo, estadoLuzInstantaneo, estadoSonidoBloqueado, estadoLuzBloqueado});

        digitalWrite(LED_VERDE_PIN, currentLedStatus == 1);
        digitalWrite(LED_NARANJA_PIN, currentLedStatus == 2);
        digitalWrite(LED_ROJO_PIN, currentLedStatus == 3);
        
        // --- Lógica de DFPlayer con Temporizador ---
        if (currentLedStatus >= 2) {
            if (!isPlaying) {
                myDFPlayer.loop(1);
                isPlaying = true;
                musicStopTime = currentMillis + musicPlayDuration;
                Serial.println("-> DFPlayer: Iniciando reproducción por 3 minutos.");
            } else if (currentMillis >= musicStopTime - sendDataInterval) {
                musicStopTime = currentMillis + musicPlayDuration;
                Serial.println("-> DFPlayer: Ambiente aún estresante. Renovando música.");
            }
        } else {
            if (isPlaying) {
                myDFPlayer.stop();
                isPlaying = false;
                musicStopTime = 0;
                Serial.println("-> DFPlayer: Ambiente en calma. Deteniendo reproducción.");
            }
        }

        // --- Salida al Monitor Serie ---
        Serial.print("Temp: "); Serial.print(temperature);
        Serial.print(" *C, Sonido: "); Serial.print(soundLevel);
        Serial.print(", Luz: "); Serial.print(lightLevel);
        Serial.print(" -> Estado LED: "); Serial.print(currentLedStatus);
        printStateReason(currentLedStatus, estadoTemp, estadoSonidoInstantaneo, estadoLuzInstantaneo, estadoSonidoBloqueado, estadoLuzBloqueado);
        
        // --- Enviar datos a Firebase ---
        if (firebaseReady && (currentMillis % firebaseUpdateInterval < sendDataInterval)) {
            FirebaseJson json;
            json.set("temperature", temperature);
            json.set("humidity", humidity);
            json.set("soundLevel", soundLevel);
            json.set("lightLevel", lightLevel);
            json.set("ledStatus", currentLedStatus);
            Firebase.RTDB.setJSON(&fbdo, sensorDataPath, &json);
        }
    }
    
    // --- Mantenimiento DFPlayer ---
    if (myDFPlayer.available()) {
      // printDetail(myDFPlayer.readType(), myDFPlayer.read()); 
    }
}
