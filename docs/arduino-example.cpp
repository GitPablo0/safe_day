
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

// ===============================================================
// 1. CONFIGURACIÓN DE USUARIO Y WIFI
// ===============================================================
// ¡¡¡ PEGA TU UID AQUÍ !!! (Debe ser "ZqJgY6f4n3X2bE8c5hT9kLpWvA1s" para que la app lo vea)
#define USER_UID "PASTE_YOUR_USER_UID_HERE"

#define WIFI_SSID "Telecentro-416b"
#define WIFI_PASSWORD "ATNMFMY2MM5Q"

// ===============================================================
// 2. CONFIGURACIÓN DE FIREBASE
// ===============================================================
#define API_KEY "AIzaSyCMmir0FR57YVoVGQ9QCUG4oG7bUEF3w7I"
#define FIREBASE_PROJECT_ID "studio-4253330981-dcdc9"
#define DATABASE_URL "studio-4253330981-dcdc9.firebaseio.com"

// Objetos de Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===============================================================
// 3. PINES Y SENSORES
// ===============================================================
#define DHTPIN 4
#define DHTTYPE DHT22
const int SOUND_SENSOR_PIN = 34;

DHT dht(DHTPIN, DHTTYPE);

const int LED_VERDE_PIN = 32;
const int LED_NARANJA_PIN = 33;
const int LED_ROJO_PIN = 25;

// Umbrales (se actualizarán desde Firebase)
float soundThresholdLow = 300;
float soundThresholdMedium = 380;
float tempThresholdVerde = 24;
float tempThresholdNaranja = 28;

// ===============================================================
// 4. LÓGICA DE LA APLICACIÓN
// ===============================================================
unsigned long sendDataPrevMillis = 0;
unsigned long fetchConfigPrevMillis = 0;
const long sendDataInterval = 5000;
const long fetchConfigInterval = 60000;

// Declaraciones anticipadas
void fetchThresholdsFromFirebase();

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando...");

  dht.begin();
  delay(2000);

  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_NARANJA_PIN, OUTPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  digitalWrite(LED_VERDE_PIN, LOW);
  digitalWrite(LED_NARANJA_PIN, LOW);
  digitalWrite(LED_ROJO_PIN, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Conectado con IP: ");
  Serial.println(WiFi.localIP());
  String deviceId = WiFi.macAddress();
  deviceId.replace(":", "");
  Serial.print("Device ID: ");
  Serial.println(deviceId);

  Serial.println("Configurando Firebase...");
  
  // Asignamos las credenciales a la configuración
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Para la versión 4.4.17, necesitamos indicar que se debe obtener un token anónimo
  // Esto se logra dejando el UID del service account vacío y no usando credenciales de servicio.
  // La biblioteca intentará obtener un token de acceso anónimo usando la API Key.
  auth.user.email = "";
  auth.user.password = "";

  Firebase.reconnectWiFi(true);
  
  // La función begin correcta que usa los objetos de configuración
  Firebase.begin(&config, &auth);

  // Intento inicial de obtener la configuración.
  fetchThresholdsFromFirebase();
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

void fetchThresholdsFromFirebase() {
    // Usamos strnlen para verificar de forma segura que el UID no esté vacío o sea el placeholder
    if (strnlen(USER_UID, 30) < 2 || strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
        Serial.println("No hay UID configurado para obtener umbrales.");
        return;
    }

    // Comprobamos si Firebase está listo
    if (!Firebase.ready()) {
        Serial.println("Firebase no está listo para obtener umbrales.");
        return;
    }

    String docPath = "users/" + String(USER_UID) + "/thresholdConfigurations/default_config";
    Serial.println("Obteniendo documento de configuración desde: " + docPath);

    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", docPath.c_str())) {
        if (fbdo.httpCode() == 200) {
            FirebaseJson json(fbdo.payload());
            FirebaseJsonData result;

            if (json.get(result, "fields/soundThresholdLow/doubleValue") || json.get(result, "fields/soundThresholdLow/integerValue")) soundThresholdLow = result.doubleValue;
            if (json.get(result, "fields/soundThresholdMedium/doubleValue") || json.get(result, "fields/soundThresholdMedium/integerValue")) soundThresholdMedium = result.doubleValue;
            if (json.get(result, "fields/temperatureThresholdVerde/doubleValue") || json.get(result, "fields/temperatureThresholdVerde/integerValue")) tempThresholdVerde = result.doubleValue;
            if (json.get(result, "fields/temperatureThresholdNaranja/doubleValue") || json.get(result, "fields/temperatureThresholdNaranja/integerValue")) tempThresholdNaranja = result.doubleValue;

            Serial.println("Umbrales actualizados desde Firebase:");
            Serial.printf("  - Sonido (Naranja): %.0f, Sonido (Rojo): %.0f\n", soundThresholdLow, soundThresholdMedium);
            Serial.printf("  - Temp (Naranja): %.1f, Temp (Rojo): %.1f\n", tempThresholdVerde, tempThresholdNaranja);
        } else {
            Serial.printf("Error al obtener documento de configuración: %s\n", fbdo.errorReason().c_str());
        }
    } else {
        Serial.printf("Fallo en la llamada a getDocument para la configuración: %s\n", fbdo.errorReason().c_str());
    }
}


void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - fetchConfigPrevMillis >= fetchConfigInterval) {
        fetchConfigPrevMillis = currentMillis;
        fetchThresholdsFromFirebase();
    }

    if (currentMillis - sendDataPrevMillis >= sendDataInterval) {
        sendDataPrevMillis = currentMillis;

        float temperature = readTemperature();
        float humidity = readHumidity();
        int soundLevel = readSoundLevel();

        Serial.println("------------------------------------");
        Serial.printf("Temp: %.1f, Hum: %.1f, Sonido: %d\n", temperature, humidity, soundLevel);

        int estadoSonido = 1;
        if (soundLevel >= soundThresholdLow) estadoSonido = 2;
        if (soundLevel >= soundThresholdMedium) estadoSonido = 3;

        int estadoTemp = 1;
        if (temperature >= tempThresholdVerde) estadoTemp = 2;
        if (temperature >= tempThresholdNaranja) estadoTemp = 3;

        int ledStatus = max(estadoSonido, estadoTemp);

        digitalWrite(LED_VERDE_PIN, ledStatus == 1);
        digitalWrite(LED_NARANJA_PIN, ledStatus == 2);
        digitalWrite(LED_ROJO_PIN, ledStatus == 3);

        if (strnlen(USER_UID, 30) < 2 || strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
            Serial.println("No hay UID configurado para enviar datos.");
            return;
        }

        if (!Firebase.ready()) {
            Serial.println("Firebase no está listo para enviar datos.");
            return;
        }
        
        String documentPath = "users/" + String(USER_UID) + "/sensorData/latest";
        String content = "{\"fields\":{\"temperature\":{\"doubleValue\":" + String(temperature, 1) + "},\"humidity\":{\"doubleValue\":" + String(humidity, 1) + "},\"soundLevel\":{\"integerValue\":" + String(soundLevel) + "},\"timestamp\":{\"timestampValue\":{\".sv\":\"timestamp\"}}}}";

        // La función patchDocument requiere 6 argumentos en esta versión de la librería.
        // El último argumento es el "update mask", que podemos dejar vacío.
        if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.c_str(), "" /* <- Argumento de máscara requerido */)) {
            Serial.println("Datos enviados a Firestore exitosamente.");
        } else {
            Serial.printf("Error al enviar datos: %s\n", fbdo.errorReason().c_str());
        }
    }
}

    