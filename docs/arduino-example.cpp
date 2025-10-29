/**
 * ==================================================================================
 * Este es un archivo de ejemplo para conectar un microcontrolador (como un ESP32)
 * a Firebase Firestore para el proyecto NeuroSense.
 *
 * Dependencias de la biblioteca de Arduino (Gestor de Bibliotecas):
 * 1. Firebase Arduino Client Library for ESP32 and ESP8266 by Mobizt
 * 2. ArduinoJson by Benoit Blanchon
 * 3. DHT sensor library by Adafruit
 *
 * --- INSTRUCCIONES ---
 * 1. Abre la aplicación web para generar un usuario anónimo.
 * 2. Ve a tu consola de Firebase -> Authentication -> Users.
 * 3. Copia el "User UID" del nuevo usuario anónimo.
 * 4. Pega ese UID en la variable `USER_UID` de este archivo.
 * 5. Carga el código en tu ESP32.
 * ==================================================================================
 */
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include "DHT.h"

// ===============================================================
// 1. CONFIGURACIÓN DE WIFI
// ===============================================================
#define WIFI_SSID "Telecentro-416b"
#define WIFI_PASSWORD "ATNMFMY2MM5Q"

// ===============================================================
// 2. CONFIGURACIÓN DE FIREBASE
// ===============================================================
#define API_KEY "AIzaSyCMmir0FR57YVoVGQ9QCUG4oG7bUEF3w7I"
#define FIREBASE_PROJECT_ID "studio-4253330981-dcdc9"
// IMPORTANTE: Reemplaza con el UID de tu usuario anónimo de Firebase
#define USER_UID "pega-tu-uid-aqui"

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

const int LED_VERDE_PIN = 12;
const int LED_NARANJA_PIN = 13;
const int LED_ROJO_PIN = 14;

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
const long sendDataInterval = 5000;    // 5 segundos
const long fetchConfigInterval = 60000; // 60 segundos

void fetchThresholdsFromFirebase(); // Declaración anticipada

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
  Serial.println();
  
  Serial.println("Sincronizando hora...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nHora sincronizada correctamente.");

  Serial.println("Configurando Firebase...");
  config.api_key = API_KEY;
  
  auth.user.email = "";
  auth.user.password = "";
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Esperando token de autenticación...");
  while (!Firebase.ready()) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nFirebase listo!");

  // Obtener la configuración inicial al arrancar
  fetchThresholdsFromFirebase();
}

float readTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {
    return 0; 
  }
  return t;
}

float readHumidity() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    return 0;
  }
  return h;
}

int readSoundLevel() {
  const int sampleWindow = 50;
  unsigned int sample;
  unsigned long startMillis = millis(); 
  unsigned int peakToPeak = 0;
  unsigned int signalMax = 0;
  unsigned int signalMin = 4095;

  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(SOUND_SENSOR_PIN);
    if (sample < 4095) {
      if (sample > signalMax) {
        signalMax = sample;
      } else if (sample < signalMin) {
        signalMin = sample;
      }
    }
  }
  peakToPeak = signalMax - signalMin;
  return peakToPeak;
}

void fetchThresholdsFromFirebase() {
    if (!Firebase.ready()) {
        Serial.println("Firebase no está listo, no se pueden obtener los umbrales.");
        return;
    }

    String documentPath = "users/" + String(USER_UID) + "/thresholdConfigurations";
    Serial.println("Obteniendo umbrales desde: " + documentPath);

    // Listamos los documentos de la subcolección para obtener el ID del documento de configuración
    if (Firebase.Firestore.listDocuments(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), "")){
        if (fbdo.httpCode() == 200) {
            StaticJsonDocument<1024> listDoc;
            deserializeJson(listDoc, fbdo.payload());
            if (listDoc["documents"] && listDoc["documents"][0]) {
                String fullDocName = listDoc["documents"][0]["name"];
                // El nombre completo es "projects/PROJECT_ID/databases/(default)/documents/users/UID/thresholdConfigurations/DOC_ID"
                // Necesitamos extraer solo el path relativo para getDocument.
                int docPathIndex = fullDocName.indexOf("/documents/");
                String docPath = fullDocName.substring(docPathIndex + 10);
                
                Serial.println("Obteniendo documento: " + docPath);
                
                // Ahora obtenemos el documento específico
                if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", docPath.c_str())) {
                    if (fbdo.httpCode() == 200) {
                        String payload = fbdo.payload().c_str();
                        StaticJsonDocument<1024> doc;
                        DeserializationError error = deserializeJson(doc, payload);

                        if (error) {
                            Serial.print("deserializeJson() falló: ");
                            Serial.println(error.c_str());
                            return;
                        }

                        JsonObject fields = doc["fields"];
                        if (fields.containsKey("soundThresholdLow")) {
                            soundThresholdLow = fields["soundThresholdLow"]["integerValue"].as<float>();
                        }
                        if (fields.containsKey("soundThresholdMedium")) {
                            soundThresholdMedium = fields["soundThresholdMedium"]["integerValue"].as<float>();
                        }
                        if (fields.containsKey("temperatureThresholdVerde")) {
                            tempThresholdVerde = fields["temperatureThresholdVerde"]["doubleValue"].as<float>();
                        }
                        if (fields.containsKey("temperatureThresholdNaranja")) {
                            tempThresholdNaranja = fields["temperatureThresholdNaranja"]["doubleValue"].as<float>();
                        }
                        
                        Serial.println("Umbrales actualizados desde Firebase:");
                        Serial.printf("  - Sonido (Naranja): %.0f, Sonido (Rojo): %.0f\n", soundThresholdLow, soundThresholdMedium);
                        Serial.printf("  - Temp (Naranja): %.1f, Temp (Rojo): %.1f\n", tempThresholdVerde, tempThresholdNaranja);
                    } else {
                        Serial.printf("Error al obtener documento: %s\n", fbdo.errorReason().c_str());
                    }
                } else {
                    Serial.printf("Error en Firebase.Firestore.getDocument: %s\n", fbdo.errorReason().c_str());
                }
            } else {
                Serial.println("No se encontraron documentos de configuración en la subcolección.");
            }
        } else {
             Serial.printf("Error al listar documentos: %s\n", fbdo.errorReason().c_str());
        }
    } else {
        Serial.printf("Error en Firebase.Firestore.listDocuments: %s\n", fbdo.errorReason().c_str());
    }
}


void loop() {
    unsigned long currentMillis = millis();

    // Tarea: Obtener configuración de umbrales desde Firebase periódicamente
    if (currentMillis - fetchConfigPrevMillis > fetchConfigInterval) {
        fetchConfigPrevMillis = currentMillis;
        fetchThresholdsFromFirebase();
    }

    // Tarea: Enviar datos de sensores a Firebase periódicamente
    if (currentMillis - sendDataPrevMillis > sendDataInterval) {
        sendDataPrevMillis = currentMillis;

        float temperature = readTemperature();
        float humidity = readHumidity();
        int soundLevel = readSoundLevel();

        Serial.println("------------------------------------");
        Serial.println("Intentando enviar datos a Firebase...");
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
        Serial.printf("Estado de LEDs determinado: %d (Sonido: %d, Temp: %d)\n", ledStatus, estadoSonido, estadoTemp);

        String documentPath = "sensorData/latest";
        String content = "{'fields': {'temperature': {'doubleValue': " + String(temperature) + "}, 'humidity': {'doubleValue': " + String(humidity) + "}, 'soundLevel': {'integerValue': " + String(soundLevel) + "}}}";

        if (Firebase.ready()) {
            // El último argumento es el "updateMask", lo dejamos vacío para actualizar todo.
            if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.c_str(), "")) {
                // Silencio en éxito para no llenar la consola
            } else {
                Serial.printf("Error al enviar datos: %s\n", fbdo.errorReason().c_str());
            }
        } else {
            Serial.println("Firebase no está listo. Comprueba la conexión y las credenciales.");
        }
    }
}
