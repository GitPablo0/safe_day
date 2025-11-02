
/**
 * ==================================================================================
 * Este es un archivo de ejemplo para conectar un microcontrolador (como un ESP32)
 * a Firebase Firestore para el proyecto NeuroSense.
 *
 * --- ¡IMPORTANTE! VERSIÓN DE BIBLIOTECA ---
 * Este código está diseñado para la versión 5.x.x o superior de la biblioteca
 * "Firebase Arduino Client Library for ESP32 and ESP8266" by Mobizt.
 * Asegúrate de tener la última versión instalada desde el Gestor de Bibliotecas.
 *
 * Dependencias de la biblioteca de Arduino (Gestor de Bibliotecas):
 * 1. Firebase Arduino Client Library for ESP32 and ESP8266 by Mobizt (v5.x.x+)
 * 2. ArduinoJson by Benoit Blanchon (v7.x)
 * 3. DHT sensor library by Adafruit
 *
 * --- INSTRUCCIONES ---
 * 1. Carga este código en tu ESP32.
 * 2. Abre el Monitor Serie (115200 baudios).
 * 3. Si el dispositivo no está vinculado, verás un "Token de Vinculación".
 * 4. Ingresa ese token en la aplicación web para vincular el dispositivo a tu cuenta.
 * 5. Una vez vinculado, el dispositivo funcionará normalmente.
 * ==================================================================================
 */
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <EEPROM.h>

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

String USER_UID = "";

// Objetos de Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===============================================================
// 3. CONFIGURACIÓN DE EEPROM Y VINCULACIÓN
// ===============================================================
#define EEPROM_SIZE 128
#define UID_ADDR 0
#define UID_LEN 64
#define FLAG_ADDR 100

String deviceId = "";
String pairingDocPath = "";
unsigned long lastPairingCheckMillis = 0;
bool pairingDocCreated = false;

// ===============================================================
// 4. PINES Y SENSORES
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
// 5. LÓGICA DE LA APLICACIÓN
// ===============================================================
unsigned long sendDataPrevMillis = 0;
unsigned long fetchConfigPrevMillis = 0;
const long sendDataInterval = 5000;
const long fetchConfigInterval = 60000;
bool isPaired = false;

// Declaraciones anticipadas
void fetchThresholdsFromFirebase();
void checkPairingStatus();
void createPairingDocument();

// Función de callback para el estado del token
void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_ready) {
    Serial.println("Token listo. El dispositivo está autenticado y listo para operar.");
  } else {
    Serial.printf("Estado del Token: %s, Error: %s\n",
                  info.status == token_status_error ? "error" : "desconocido",
                  info.error.message.c_str());
  }
}

// Función para leer de EEPROM
String readStringFromEEPROM(int addr, int len) {
  String str = "";
  for (int i = 0; i < len; ++i) {
    byte c = EEPROM.read(addr + i);
    if (c == 0) break;
    str += (char)c;
  }
  return str;
}

// Función para escribir en EEPROM
void writeStringToEEPROM(int addr, const String& str, int maxLen) {
  for (int i = 0; i < maxLen; ++i) {
    if (i < str.length()) {
      EEPROM.write(addr + i, str[i]);
    } else {
      EEPROM.write(addr + i, 0); // Null terminator
    }
  }
  EEPROM.commit();
}


void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando...");

  EEPROM.begin(EEPROM_SIZE);
  
  dht.begin();
  delay(2000);

  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_NARANJA_PIN, OUTPUT);
  pinMode(LED_ROJO_PIN, OUTPUT); // Asegurarse que el pin 25 es de salida
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
  deviceId = WiFi.macAddress();
  deviceId.replace(":", "");
  Serial.print("Device ID: ");
  Serial.println(deviceId);
  pairingDocPath = "devicePairing/" + deviceId;

  // Configurar la hora para la conexión segura con Firebase
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Sincronizando hora");
  while (time(nullptr) < 1510644967) {
      Serial.print(".");
      delay(1000);
  }
  Serial.println("\nHora sincronizada.");

  Serial.println("Configurando Firebase...");
  config.api_key = API_KEY;
  config.token_status_callback = tokenStatusCallback;

  // Autenticación anónima
  auth.user.email = "";
  auth.user.password = "";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  if (Firebase.signUpAnonymous(&config, &auth)) {
    Serial.println("Autenticación anónima exitosa.");
  } else {
    Serial.printf("Fallo en la autenticación anónima: %s\n", config.signer.tokens.error.message.c_str());
  }

  // Comprobar si el dispositivo ya está vinculado
  if (EEPROM.read(FLAG_ADDR) == 1) {
    USER_UID = readStringFromEEPROM(UID_ADDR, UID_LEN);
    if (USER_UID.length() > 0) {
      isPaired = true;
      Serial.println("Dispositivo ya vinculado con UID guardado. Operando normalmente.");
      fetchThresholdsFromFirebase();
    }
  }

  if (!isPaired) {
    Serial.println("Dispositivo no vinculado. Iniciando modo de vinculación...");
  }
}

void createPairingDocument() {
    String pairingToken = String(random(100000, 999999));
    Serial.println("==========================================");
    Serial.println("            MODO DE VINCULACIÓN           ");
    Serial.println(" Ingrese el siguiente token en la aplicación web:");
    Serial.print(" Token de Vinculación: ");
    Serial.println(pairingToken);
    Serial.println("==========================================");

    String content = "{\"pairingToken\": {\"stringValue\": \"" + pairingToken + "\"}, \"userId\": {\"nullValue\": null}}";
    
    if (Firebase.Firestore.createDocument(fbdo, FIREBASE_PROJECT_ID, "", pairingDocPath.c_str(), content.c_str())) {
        Serial.println("Documento de vinculación creado. Esperando que el usuario lo reclame...");
        pairingDocCreated = true;
    } else {
        Serial.printf("Error al crear documento de vinculación: %s\n", fbdo.errorReason().c_str());
    }
}

void checkPairingStatus() {
  Serial.println("Comprobando estado de vinculación...");
  if (Firebase.Firestore.getDocument(fbdo, FIREBASE_PROJECT_ID, "", pairingDocPath.c_str())) {
      if (fbdo.httpCode() == 200) {
          FirebaseJson json(fbdo.payload());
          FirebaseJsonData result;
          json.get(result, "fields/userId/stringValue");

          if (result.success && result.stringValue.length() > 0) {
              String receivedUid = result.stringValue;
              Serial.println("¡Dispositivo vinculado exitosamente!");
              Serial.print("User UID recibido: ");
              Serial.println(receivedUid);

              writeStringToEEPROM(UID_ADDR, receivedUid, UID_LEN);
              EEPROM.write(FLAG_ADDR, 1);
              EEPROM.commit();

              Firebase.Firestore.deleteDocument(fbdo, FIREBASE_PROJECT_ID, "", pairingDocPath.c_str());

              Serial.println("Configuración guardada. Reiniciando en 3 segundos...");
              delay(3000);
              ESP.restart();
          } else {
            Serial.println("El documento existe pero aún no ha sido reclamado.");
          }
      } else {
        Serial.printf("Error al obtener documento de vinculación: %s\n", fbdo.errorReason().c_str());
      }
  } else {
     Serial.printf("Error en la llamada a getDocument: %s\n", fbdo.errorReason().c_str());
  }
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
    if (!Firebase.ready() || USER_UID.length() == 0) {
        Serial.println("Firebase no está listo o no hay UID para obtener umbrales.");
        return;
    }
    
    String docPath = "users/" + USER_UID + "/thresholdConfigurations/default_config";
    Serial.println("Obteniendo documento de configuración desde: " + docPath);

    if (Firebase.Firestore.getDocument(fbdo, FIREBASE_PROJECT_ID, "", docPath.c_str())) {
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
    if (!isPaired) {
        if (millis() - lastPairingCheckMillis > 10000) {
          lastPairingCheckMillis = millis();
          if(Firebase.ready()) {
            if (!pairingDocCreated) {
              createPairingDocument();
            } else {
              checkPairingStatus();
            }
          } else {
            Serial.println("Firebase no está listo para iniciar vinculación.");
          }
        }
        delay(100);
        return;
    }

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
        
        String documentPath = "users/" + USER_UID + "/sensorData/latest";
        String content = "{\"temperature\": {\"doubleValue\": " + String(temperature, 1) + "}, \"humidity\": {\"doubleValue\": " + String(humidity, 1) + "}, \"soundLevel\": {\"integerValue\": " + String(soundLevel) + "}, \"timestamp\": {\"timestampValue\": \"" + fbdo.host() + "\"}}";

        if (Firebase.ready()) {
            if (!Firebase.Firestore.patchDocument(fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.c_str())) {
                Serial.printf("Error al enviar datos: %s\n", fbdo.errorReason().c_str());
            }
        } else {
            Serial.println("Firebase no está listo para enviar datos.");
        }
    }
}

    