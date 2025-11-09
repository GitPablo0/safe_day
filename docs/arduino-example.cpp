/**
 * ==================================================================================
 * Este es un archivo de ejemplo para conectar un microcontrolador (como un ESP32)
 * a Firebase Firestore para el proyecto NeuroSense.
 *
 * --- ¡IMPORTANTE! INSTRUCCIONES DE UID MANUAL ---
 * 1. Abre la aplicación web del dashboard.
 * 2. Copia el "Device UID" que aparece en la tarjeta "Device Sync".
 * 3. Pega ese UID en la línea de abajo, reemplazando "PASTE_YOUR_USER_UID_HERE".
 *
 * --- ¡IMPORTANTE! VERSIÓN DE BIBLIOTECA ---
 * Este código está diseñado para la versión 4.4.17 de la biblioteca
 * "Firebase Arduino Client Library for ESP32 and ESP8266" by Mobizt.
 * ==================================================================================
 */
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include "time.h"
#include <HardwareSerial.h>
#include "DFRobotDFPlayerMini.h"

// ===============================================================
// 1. CONFIGURACIÓN DE USUARIO Y WIFI
// ===============================================================
// ¡¡¡ PEGA TU UID AQUÍ !!!
#define USER_UID "eOp4B72C0wdVqf6PZDOOQGGYRdP2"

#define WIFI_SSID "Telecentro-416b"
#define WIFI_PASSWORD "ATNMFMY2MM5Q"

// ===============================================================
// 2. CONFIGURACIÓN DE FIREBASE
// ===============================================================
#define API_KEY "AIzaSyCMmir0FR57YVoVGQ9QCUG4oG7bUEF3w7I"
#define FIREBASE_PROJECT_ID "studio-4253330981-dcdc9"
#define DATABASE_URL "studio-4253330981-dcdc9-default-rtdb.firebaseio.com"

// Objetos de Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===============================================
// 3. PINES Y SENSORES
// ===============================================
#define DHTPIN 4
#define DHTTYPE DHT22
const int SOUND_SENSOR_PIN = 34;
const int LIGHT_SENSOR_PIN = 36; 

DHT dht(DHTPIN, DHTTYPE);

const int LED_VERDE_PIN = 32;
const int LED_NARANJA_PIN = 33;
const int LED_ROJO_PIN = 25;

// Configuración DFPlayer Mini
HardwareSerial mySerial(2);
DFRobotDFPlayerMini myDFPlayer;

// ===============================================
// 4. UMBRALES Y LÓGICA DE EVENTOS (se actualizan desde Firebase)
// ===============================================
// Sonido
float soundThresholdLow = 300;
float soundThresholdMedium = 380;
int orangeSoundEventCountThreshold = 3;
int redSoundEventCountThreshold = 3;
long timeBetweenEventsMillis = 10000;
long orangeBlockDurationMillis = 60000;
long redBlockDurationMillis = 60000;

// Luz (Lógica inversa: valores MÁS BAJOS indican MÁS oscuridad/estrés)
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
// 5. VARIABLES DE ESTADO
// ===============================================
unsigned long sendDataPrevMillis = 0;
unsigned long fetchConfigPrevMillis = 0;
unsigned long playerStatusPrevMillis = 0;
unsigned long fetchCommandsPrevMillis = 0;

const long sendDataInterval = 1000;
const long fetchConfigInterval = 60000;
const long playerStatusInterval = 5000; // Intervalo para reportar estado del reproductor
const long fetchCommandsInterval = 3000; // Intervalo para buscar nuevos comandos


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
bool isPlayingMusic = false;
unsigned long musicStopTime = 0;

unsigned long lastCommandTimestamp = 0; // Para evitar comandos repetidos

// Declaraciones anticipadas
void fetchThresholdsFromFirebase();
void reportPlayerStatus();
void fetchMusicCommands();

// Función de callback para el estado del token
void tokenStatusCallback(TokenInfo info) {
    if (info.status == token_status_ready) {
        Serial.println("Estado del Token: listo");
        // Una vez que el token está listo, podemos obtener la configuración inicial.
        fetchThresholdsFromFirebase();
    } else {
        Serial.printf("Estado del Token: error, %s\n", info.error.message.c_str());
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Inicializando...");

    dht.begin();
    mySerial.begin(9600, SERIAL_8N1, 27, 26); 
    delay(2000);

    pinMode(LED_VERDE_PIN, OUTPUT);
    pinMode(LED_NARANJA_PIN, OUTPUT);
    pinMode(LED_ROJO_PIN, OUTPUT);
    digitalWrite(LED_VERDE_PIN, LOW);
    digitalWrite(LED_NARANJA_PIN, LOW);
    digitalWrite(LED_ROJO_PIN, LOW);

    for (int i = 0; i < MAX_SOUND_EVENTS; i++) {
        orangeEventTimestamps[i] = 0;
        redEventTimestamps[i] = 0;
    }
    for (int i = 0; i < MAX_LIGHT_EVENTS; i++) {
        orangeLightEventTimestamps[i] = 0;
        redLightEventTimestamps[i] = 0;
    }
    
    Serial.println("Iniciando DFPlayer...");
    if (!myDFPlayer.begin(mySerial)) { 
        Serial.println("No se pudo iniciar el DFPlayer. Verifica conexiones y tarjeta SD.");
    } else {
       Serial.println("DFPlayer Mini iniciado.");
       delay(1000); // Dar tiempo al DFPlayer para inicializar
       myDFPlayer.volume(15);
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Conectando a Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Conectado con IP: ");
    Serial.println(WiFi.localIP());

    Serial.println("Sincronizando hora...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        now = time(nullptr);
    }
    Serial.println("Hora sincronizada.");

    Serial.println("Configurando Firebase...");
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    config.signer.test_mode = true;
    config.token_status_callback = tokenStatusCallback;
    Firebase.reconnectWiFi(true);
    Firebase.begin(&config, &auth);
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

// Función auxiliar para extraer el objeto "fields" del JSON de respuesta de Firestore
String getJsonString(String& payload, const char* fieldName) {
    String searchKey = "\"" + String(fieldName) + "\":";
    int startIndex = payload.indexOf(searchKey);
    if (startIndex == -1) return "{}";
    startIndex = payload.indexOf("{", startIndex);
    int braceCount = 1;
    int endIndex = startIndex + 1;
    while (endIndex < payload.length() && braceCount > 0) {
        if (payload[endIndex] == '{') braceCount++;
        if (payload[endIndex] == '}') braceCount--;
        endIndex++;
    }
    if (endIndex > payload.length()) return "{}";
    return payload.substring(startIndex, endIndex);
}

void fetchMusicCommands() {
    if (!Firebase.ready()) return;

    String docPath = "users/" + String(USER_UID) + "/musicControl/state";
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", docPath.c_str())) {
        if (fbdo.httpCode() == 200) {
            String payload = fbdo.payload();
            String fieldsJsonString = getJsonString(payload, "fields");
            FirebaseJson json;
            json.setJsonData(fieldsJsonString);
            FirebaseJsonData result;

            if (json.get(result, "lastCommandTimestamp/integerValue") || json.get(result, "lastCommandTimestamp/doubleValue")) {
                unsigned long newTimestamp = (unsigned long)atol(result.stringValue.c_str());
                if (newTimestamp <= lastCommandTimestamp) {
                    return; // Comando ya procesado o es antiguo.
                }
                lastCommandTimestamp = newTimestamp; // Actualizar al timestamp más reciente

                // Procesar comando de volumen
                if (json.get(result, "volume/integerValue")) {
                    int newVolume = result.intValue;
                    if(newVolume >= 0 && newVolume <= 30) {
                        myDFPlayer.volume(newVolume);
                        Serial.printf("-> Comando Remoto: Volumen ajustado a %d\n", newVolume);
                    }
                }
                
                // Procesar comando de reproducción
                if (json.get(result, "command/stringValue")) {
                    String command = result.stringValue.c_str();
                    Serial.printf("-> Comando Remoto: %s\n", command.c_str());
                    if (command == "play") {
                        myDFPlayer.start();
                        isPlayingMusic = true;
                    } else if (command == "pause") {
                        myDFPlayer.pause();
                        isPlayingMusic = false;
                    } else if (command == "next") {
                        myDFPlayer.next();
                    } else if (command == "prev") {
                        myDFPlayer.previous();
                    }
                }
                reportPlayerStatus(); // Reportar estado después de ejecutar un comando
            }
        }
    } else {
         // Serial.printf("Fallo en la llamada a getDocument para comandos: %s\n", fbdo.errorReason().c_str());
    }
}


void fetchThresholdsFromFirebase() {
    if (strnlen(USER_UID, 30) < 2 || strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
        Serial.println("No hay UID configurado para obtener umbrales.");
        return;
    }
    if (!Firebase.ready()) {
        Serial.println("Firebase no está listo para obtener umbrales.");
        return;
    }
    String docPath = "users/" + String(USER_UID) + "/thresholdConfigurations/main";
    Serial.println("Obteniendo documento de configuración desde: " + docPath);
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", docPath.c_str())) {
        if (fbdo.httpCode() == 200) {
            String payload = fbdo.payload();
            String fieldsJsonString = getJsonString(payload, "fields");
            FirebaseJson json;
            json.setJsonData(fieldsJsonString);
            FirebaseJsonData result;
            
            Serial.println("Umbrales actualizados desde Firebase:");
            if (json.get(result, "soundThresholdLow/doubleValue") || json.get(result, "soundThresholdLow/integerValue")) soundThresholdLow = result.doubleValue;
            if (json.get(result, "soundThresholdMedium/doubleValue") || json.get(result, "soundThresholdMedium/integerValue")) soundThresholdMedium = result.doubleValue;
            Serial.printf("  - Sonido (Naranja): %.0f, Sonido (Rojo): %.0f\n", soundThresholdLow, soundThresholdMedium);
            
            if (json.get(result, "temperatureThresholdVerde/doubleValue") || json.get(result, "temperatureThresholdVerde/integerValue")) tempThresholdVerde = result.doubleValue;
            if (json.get(result, "temperatureThresholdNaranja/doubleValue") || json.get(result, "temperatureThresholdNaranja/integerValue")) tempThresholdNaranja = result.doubleValue;
            Serial.printf("  - Temp (Naranja): %.1f, Temp (Rojo): %.1f\n", tempThresholdVerde, tempThresholdNaranja);

            if (json.get(result, "lightThresholdLow/doubleValue") || json.get(result, "lightThresholdLow/integerValue")) lightThresholdLow = result.doubleValue;
            if (json.get(result, "lightThresholdMedium/doubleValue") || json.get(result, "lightThresholdMedium/integerValue")) lightThresholdMedium = result.doubleValue;
            Serial.printf("  - Luz (Naranja): %.0f, Luz (Rojo): %.0f\n", lightThresholdLow, lightThresholdMedium);

            if (json.get(result, "orangeSoundEventCount/integerValue")) orangeSoundEventCountThreshold = result.intValue;
            if (json.get(result, "redSoundEventCount/integerValue")) redSoundEventCountThreshold = result.intValue;
            if (json.get(result, "timeBetweenEvents/integerValue")) timeBetweenEventsMillis = result.intValue;
            if (json.get(result, "orangeBlockDuration/integerValue")) orangeBlockDurationMillis = result.intValue;
            if (json.get(result, "redBlockDuration/integerValue")) redBlockDurationMillis = result.intValue;
            Serial.printf("  - Logica Eventos Sonido: Naranja Cnt: %d, Rojo Cnt: %d, Ventana: %ldms, Bloqueo N: %ldms, Bloqueo R: %ldms\n", orangeSoundEventCountThreshold, redSoundEventCountThreshold, timeBetweenEventsMillis, orangeBlockDurationMillis, redBlockDurationMillis);

            if (json.get(result, "orangeLightEventCount/integerValue")) orangeLightEventCountThreshold = result.intValue;
            if (json.get(result, "redLightEventCount/integerValue")) redLightEventCountThreshold = result.intValue;
            if (json.get(result, "timeBetweenLightEvents/integerValue")) timeBetweenLightEventsMillis = result.intValue;
            if (json.get(result, "orangeLightBlockDuration/integerValue")) orangeLightBlockDurationMillis = result.intValue;
            if (json.get(result, "redLightBlockDuration/integerValue")) redLightBlockDurationMillis = result.intValue;
            Serial.printf("  - Logica Eventos Luz: Naranja Cnt: %d, Rojo Cnt: %d, Ventana: %ldms, Bloqueo N: %ldms, Bloqueo R: %ldms\n", orangeLightEventCountThreshold, redLightEventCountThreshold, timeBetweenLightEventsMillis, orangeLightBlockDurationMillis, redLightBlockDurationMillis);

        } else {
            Serial.printf("Error al obtener documento de configuración: %s\n", fbdo.errorReason().c_str());
        }
    } else {
        Serial.printf("Fallo en la llamada a getDocument para la configuración: %s\n", fbdo.errorReason().c_str());
    }
}

String getTimestamp() {
    time_t now;
    time(&now);
    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    return String(buf);
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

void reportPlayerStatus() {
    if (!Firebase.ready()) return;

    String status = "stopped";
    // La librería a veces devuelve 512 para play y 513 para stop. Depende de la versión del chip.
    // Usaremos isPlayingMusic como fuente de verdad, y leemos el estado solo para debug/info.
    int deviceState = myDFPlayer.readState();

    if (isPlayingMusic && deviceState != 514) { // Si nuestra lógica dice que está sonando y no está en pausa
        status = "playing";
    } else if (deviceState == 514) { // 514 es pausa
        status = "paused";
        isPlayingMusic = false; // Corregir nuestro estado si se pausó manualmente
    } else {
        status = "stopped";
        isPlayingMusic = false;
    }
    
    int currentTrack = myDFPlayer.readCurrentFileNumber();
    if (currentTrack < 0) currentTrack = 0;
    int trackCount = myDFPlayer.readFileCounts();
    if (trackCount < 0) trackCount = 0;

    String path = "users/" + String(USER_UID) + "/musicControl/state";
    String content = "{\"fields\":{\"status\":{\"stringValue\":\"" + status + "\"},\"currentTrack\":{\"integerValue\":" + String(currentTrack) + "},\"trackCount\":{\"integerValue\":" + String(trackCount) + "}}}";
    
    // Usamos patchDocument para no sobreescribir los comandos que vienen de la app
    if (!Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), content.c_str(), "status,currentTrack,trackCount")) {
        // Silencio en éxito
    }
}


void loop() {
    unsigned long currentMillis = millis();

    // Es necesario llamar a myDFPlayer.available() para que la librería procese datos internos
    if (myDFPlayer.available()) {
      // No necesitamos hacer nada aquí, solo llamar a la función
    }
    
    // Intervalo para obtener comandos de música
    if (Firebase.ready() && currentMillis - fetchCommandsPrevMillis >= fetchCommandsInterval) {
        fetchCommandsPrevMillis = currentMillis;
        fetchMusicCommands();
    }

    // Intervalo para reportar el estado del reproductor
    if (Firebase.ready() && currentMillis - playerStatusPrevMillis >= playerStatusInterval) {
        playerStatusPrevMillis = currentMillis;
        reportPlayerStatus();
    }

    // Intervalo para obtener la configuración de umbrales
    if (currentMillis - fetchConfigPrevMillis >= fetchConfigInterval) {
        fetchConfigPrevMillis = currentMillis;
        fetchThresholdsFromFirebase();
    }

    // Intervalo para leer sensores y enviar datos
    if (currentMillis - sendDataPrevMillis >= sendDataInterval) {
        sendDataPrevMillis = currentMillis;

        float temperature = readTemperature();
        float humidity = readHumidity();
        int soundLevel = readSoundLevel();
        int lightLevel = readLightLevel();
        
        // --- Lógica de Eventos de Sonido ---
        bool orangeSoundStateActive = orangeLedBlockedUntil > currentMillis;
        if (soundLevel > 0 && soundLevel >= soundThresholdLow) {
            orangeEventTimestamps[orangeEventIndex] = currentMillis;
            orangeEventIndex = (orangeEventIndex + 1) % MAX_SOUND_EVENTS;
            if (!orangeSoundStateActive && countRecentEvents(orangeEventTimestamps, timeBetweenEventsMillis, MAX_SOUND_EVENTS) >= orangeSoundEventCountThreshold) {
                orangeLedBlockedUntil = currentMillis + orangeBlockDurationMillis;
            }
        }
        
        bool redSoundStateActive = redLedBlockedUntil > currentMillis;
        if (orangeSoundStateActive && soundLevel > 0 && soundLevel >= soundThresholdMedium) {
            redEventTimestamps[redEventIndex] = currentMillis;
            redEventIndex = (redEventIndex + 1) % MAX_SOUND_EVENTS;
            if (!redSoundStateActive && countRecentEvents(redEventTimestamps, timeBetweenEventsMillis, MAX_SOUND_EVENTS) >= redSoundEventCountThreshold) {
                redLedBlockedUntil = currentMillis + redBlockDurationMillis;
                for(int i=0; i<MAX_SOUND_EVENTS; i++) { orangeEventTimestamps[i] = 0; }
                orangeEventIndex = 0;
            }
        }
        
        // --- Lógica de Eventos de Luz ---
        bool orangeLightStateActive = orangeLightLedBlockedUntil > currentMillis;
        if (lightLevel > 0 && lightLevel <= lightThresholdLow) { // Corregido: <= para "menos luz"
            orangeLightEventTimestamps[orangeLightEventIndex] = currentMillis;
            orangeLightEventIndex = (orangeLightEventIndex + 1) % MAX_LIGHT_EVENTS;
            if (!orangeLightStateActive && countRecentEvents(orangeLightEventTimestamps, timeBetweenLightEventsMillis, MAX_LIGHT_EVENTS) >= orangeLightEventCountThreshold) {
                orangeLightLedBlockedUntil = currentMillis + orangeLightBlockDurationMillis;
            }
        }

        bool redLightStateActive = redLightLedBlockedUntil > currentMillis;
        if (orangeLightStateActive && lightLevel > 0 && lightLevel <= lightThresholdMedium) { // Corregido: <= para "menos luz"
            redLightEventTimestamps[redLightEventIndex] = currentMillis;
            redLightEventIndex = (redLightEventIndex + 1) % MAX_LIGHT_EVENTS;
            if (!redLightStateActive && countRecentEvents(redLightEventTimestamps, timeBetweenLightEventsMillis, MAX_LIGHT_EVENTS) >= redLightEventCountThreshold) {
                redLedBlockedUntil = currentMillis + redLightBlockDurationMillis;
                for(int i=0; i<MAX_LIGHT_EVENTS; i++) { orangeLightEventTimestamps[i] = 0; }
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
        if (currentLedStatus >= 2 && !isPlayingMusic) {
            myDFPlayer.play(1); // Inicia la reproducción de la primera pista
            isPlayingMusic = true;
            musicStopTime = currentMillis + musicPlayDuration;
            Serial.println("-> DFPlayer: Estado de estrés detectado. Iniciando música por 3 minutos.");
            reportPlayerStatus();
        }

        // Verifica si el tiempo de reproducción ha terminado (y no fue pausado manualmente)
        if (isPlayingMusic && currentMillis >= musicStopTime) {
            myDFPlayer.stop();
            isPlayingMusic = false;
            musicStopTime = 0; // Resetea el temporizador
            Serial.println("-> DFPlayer: 3 minutos completados. Deteniendo música.");
            reportPlayerStatus();
        }

        if (strnlen(USER_UID, 30) < 2 || strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
            return;
        }

        if (!Firebase.ready()) {
            return;
        }
        
        String documentPath = "users/" + String(USER_UID) + "/sensorData/latest";
        String content = "{\"fields\":{\"temperature\":{\"doubleValue\":" + String(temperature, 1) + "},\"humidity\":{\"doubleValue\":" + String(humidity, 1) + "},\"soundLevel\":{\"integerValue\":" + String(soundLevel) + "},\"lightLevel\":{\"integerValue\":" + String(lightLevel) + "},\"ledStatus\":{\"integerValue\":" + String(currentLedStatus) + "},\"timestamp\":{\"timestampValue\":\"" + getTimestamp() + "\"}}}";

        if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.c_str(), "" )) {
             // Silencio en éxito
        } else {
            Serial.printf("Fallo en la llamada a patchDocument: %s\n", fbdo.errorReason().c_str());
        }
    }
}
