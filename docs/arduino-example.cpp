
/**
 * ==================================================================================
 * Este es un archivo de ejemplo para conectar un microcontrolador (como un ESP32)
 * a Firebase Firestore para el proyecto NeuroSense.
 *
 * --- ¡IMPORTANTE! INSTRUCCIONES DE UID MANUAL ---
 * 1. Abre la aplicación web del dashboard.
 * 2. Copia el "User UID" que aparece en la tarjeta "Device Sync".
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
// ¡¡¡ PEGA TU UID AQUÍ !!!
#define USER_UID "PASTE_YOUR_USER_UID_HERE"

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ===============================================================
// 2. CONFIGURACIÓN DE FIREBASE
// ===============================================================
#define API_KEY "YOUR_API_KEY"
#define FIREBASE_PROJECT_ID "YOUR_PROJECT_ID"
#define DATABASE_URL "YOUR_DATABASE_URL" // e.g., <PROJECT_ID>.firebaseio.com

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

// Función de callback para el estado del token
void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_ready) {
    Serial.println("Token listo. El dispositivo está autenticado y listo para operar.");
  } else {
    Serial.print("Estado del Token: error, ");
    Serial.printf("Error: %s\n", info.error.message.c_str());
  }
}

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

  Serial.print("Sincronizando hora");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (time(nullptr) < 1510644967) {
      Serial.print(".");
      delay(1000);
  }
  Serial.println("\nHora sincronizada.");

  Serial.println("Configurando Firebase...");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL; // Requerido por la biblioteca
  config.token_status_callback = tokenStatusCallback;
  
  // Para la autenticación anónima, el objeto auth puede estar vacío,
  // pero debe pasarse a Firebase.begin()
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

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
    if (!Firebase.ready() || strnlen(USER_UID, 30) < 2 || strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
        Serial.println("Firebase no está listo o no hay UID para obtener umbrales.");
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
        if (soundLevel >= soundThresholds.low) estadoSonido = 2;
        if (soundLevel >= soundThresholds.medium) estadoSonido = 3;

        int estadoTemp = 1;
        if (temperature >= tempThresholdVerde) estadoTemp = 2;
        if (temperature >= tempThresholdNaranja) estadoTemp = 3;
        
        int ledStatus = max(estadoSonido, estadoTemp);

        digitalWrite(LED_VERDE_PIN, ledStatus == 1);
        digitalWrite(LED_NARANJA_PIN, ledStatus == 2);
        digitalWrite(LED_ROJO_PIN, ledStatus == 3);
        
        String documentPath = "users/" + String(USER_UID) + "/sensorData/latest";
        String content = "{\"fields\":{\"temperature\":{\"doubleValue\":" + String(temperature, 1) + "},\"humidity\":{\"doubleValue\":" + String(humidity, 1) + "},\"soundLevel\":{\"integerValue\":" + String(soundLevel) + "},\"timestamp\":{\"timestampValue\":{\".sv\":\"timestamp\"}}}}";

        if (Firebase.ready()) {
            // El sexto argumento (máscara) es requerido por esta versión de la biblioteca.
            if (!Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.c_str(), "")) {
                Serial.printf("Error al enviar datos: %s\n", fbdo.errorReason().c_str());
            }
        } else {
            Serial.println("Firebase no está listo para enviar datos.");
        }
    }
}
