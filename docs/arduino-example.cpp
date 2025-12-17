/**
 * ==================================================================================
 * Código de control para un sistema de alarma por sonido conectado a Firebase.
 * v3 - Añadido temporizador para auto-desarmado.
 *
 * --- ¡IMPORTANTE! INSTRUCCIONES DE CONFIGURACIÓN ---
 * 1. Ve a Firebase > Authentication y crea un usuario con el email
 *    "esp32@soundalarm.com" y la contraseña "soundalarm123". Este es un paso
 *    único para que el dispositivo pueda autenticarse.
 * 2. Inicia sesión en la aplicación web con TU cuenta personal.
 * 3. Copia el "Device UID" que aparece en el panel de control.
 * 4. Pega ese UID en la constante USER_UID de abajo.
 * 5. Sube este código a tu ESP32.
 * ==================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"

// ===============================================================
// 1. CONFIGURACIÓN DE USUARIO Y WIFI
// ===============================================================
// ¡¡¡ PEGA EL UID DE TU USUARIO DE FIREBASE AQUÍ !!!
#define USER_UID "PASTE_YOUR_USER_UID_HERE"

struct WiFiNetwork {
  const char* ssid;
  const char* password;
};

WiFiNetwork knownNetworks[] = {
  {"Telecentro-416b", "ATNMFMY2MM5Q"},
  {"Pablooo13", "brenda123"},
  {"Estudiantes", "Escuelas_2025"}
};
const int numKnownNetworks = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

// ===============================================================
// 2. CONFIGURACIÓN DE FIREBASE (NO TOCAR)
// ===============================================================
#define API_KEY "AIzaSyALvpGn2QsEJOxuG-3xaQehba-cSdRYbVQ"
#define FIREBASE_PROJECT_ID "sistemababyalarm-3851454-f8120"
#define DATABASE_URL "https://sistemababyalarm-3851454-f8120-default-rtdb.firebaseio.com"

// ===============================================
// 3. PINES Y SENSORES
// ===============================================
#define SOUND_PIN 27      // Pin para el sensor de sonido digital (DO)
#define LED_ROJO_PIN 4    // Pin para el LED rojo (detección de sonido)
#define LED_AZUL_PIN 2    // Pin para el LED azul (sistema armado)

// ===============================================
// 4. OBJETOS Y VARIABLES DE ESTADO
// ===============================================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variables de estado del sistema
bool isArmed = false;
bool soundDetected = false;
int alarmDurationMinutes = 10; // Duración por defecto del temporizador
unsigned long armingTimestamp = 0; // Momento en que se armó la alarma
unsigned long lastSoundDetectedTime = 0;
const unsigned long SOUND_COOLDOWN = 3000; // 3 segundos de espera

int lastSoundSensorState = HIGH; 

unsigned long lastFirestoreRead = 0;
const long firestoreReadInterval = 3000; // Leer estado desde Firestore cada 3 segundos

// ===============================================
// 5. DECLARACIÓN DE FUNCIONES
// ===============================================
void connectToWiFi();
bool syncTime();
void tokenStatusCallback(TokenInfo info);
void readAlarmStateFromFirestore();
void sendFirestoreUpdate(String content, String updateMask);
void checkTimer();

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando Sistema de Alarma por Sonido v3...");

  pinMode(SOUND_PIN, INPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  pinMode(LED_AZUL_PIN, OUTPUT);

  digitalWrite(LED_ROJO_PIN, LOW);
  digitalWrite(LED_AZUL_PIN, LOW);

  connectToWiFi();

  if (!syncTime()) {
    Serial.println("Error crítico de NTP. Reiniciando en 5 segundos...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Configurando Firebase...");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  auth.user.email = "esp32@soundalarm.com";
  auth.user.password = "soundalarm123";

  config.token_status_callback = tokenStatusCallback;
  
  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);

  Serial.println("Sistema de Alarma por Sonido Inicializado.");
}

void loop() {
  unsigned long currentMillis = millis();

  if (!Firebase.ready() || strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
    if (strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
        Serial.println("UID de usuario no configurado. Pega el UID en el código.");
        delay(5000);
    }
    return;
  }

  if (currentMillis - lastFirestoreRead > firestoreReadInterval) {
    lastFirestoreRead = currentMillis;
    readAlarmStateFromFirestore();
  }

  checkTimer(); // Comprobar si el temporizador debe desarmar la alarma

  int currentSoundState = digitalRead(SOUND_PIN);

  if (isArmed && currentSoundState == LOW && lastSoundSensorState == HIGH) {
    if (currentMillis - lastSoundDetectedTime > SOUND_COOLDOWN) {
        Serial.println("¡SONIDO DETECTADO!");
        soundDetected = true;
        String content = "{\"fields\":{\"soundDetected\":{\"booleanValue\":true}}}";
        sendFirestoreUpdate(content, "soundDetected");
        lastSoundDetectedTime = currentMillis;
        
        digitalWrite(LED_ROJO_PIN, HIGH);
    }
  }
  
  lastSoundSensorState = currentSoundState;

  digitalWrite(LED_AZUL_PIN, isArmed);
}

// Comprueba si el temporizador de la alarma ha expirado
void checkTimer() {
  if (isArmed && alarmDurationMinutes > 0 && armingTimestamp > 0) {
    unsigned long elapsedTime = millis() - armingTimestamp;
    if (elapsedTime >= (unsigned long)alarmDurationMinutes * 60 * 1000) {
      Serial.printf("Temporizador de %d minutos finalizado. Desarmando alarma.\n", alarmDurationMinutes);
      isArmed = false;
      armingTimestamp = 0; // Resetear el temporizador
      String content = "{\"fields\":{\"isArmed\":{\"booleanValue\":false},\"soundDetected\":{\"booleanValue\":false},\"lastArmedTimestamp\":{\"nullValue\":null}}}";
      sendFirestoreUpdate(content, "isArmed,soundDetected,lastArmedTimestamp");
      digitalWrite(LED_ROJO_PIN, LOW); // Apagar LED de detección
    }
  }
}

// Función para leer el estado de la alarma desde Firestore
void readAlarmStateFromFirestore() {
  String documentPath = "users/" + String(USER_UID) + "/soundAlarm/state";
  
  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
    if (fbdo.httpCode() == 200) {
      FirebaseJson json;
      json.setJsonData(fbdo.payload());
      FirebaseJsonData result;

      if (json.get(result, "fields/isArmed/booleanValue")) {
        if(result.success) {
            bool newArmedState = result.to<bool>();
            if (isArmed != newArmedState) {
                isArmed = newArmedState;
                Serial.printf("Estado 'isArmed' actualizado desde Firestore a: %s\n", isArmed ? "ARMADO" : "DESARMADO");
                
                if (isArmed) {
                  armingTimestamp = millis(); // Iniciar temporizador al armar
                } else {
                  armingTimestamp = 0; // Detener temporizador al desarmar
                  soundDetected = false;
                  digitalWrite(LED_ROJO_PIN, LOW);
                }
            }
        }
      }
      
      if (json.get(result, "fields/alarmDuration/integerValue") || json.get(result, "fields/alarmDuration/doubleValue")) {
        if(result.success) {
          int newDuration = result.to<int>();
          if (alarmDurationMinutes != newDuration) {
            alarmDurationMinutes = newDuration;
            Serial.printf("Duración de la alarma actualizada a: %d minutos\n", alarmDurationMinutes);
          }
        }
      }
      
    } else {
      Serial.printf("Error al leer documento. HTTP: %d, Razón: %s\n", fbdo.httpCode(), fbdo.errorReason().c_str());
    }
  } else {
    Serial.println("Fallo en getDocument: " + fbdo.errorReason());
  }
}

// Función genérica para enviar actualizaciones a Firestore
void sendFirestoreUpdate(String content, String updateMask) {
  String documentPath = "users/" + String(USER_UID) + "/soundAlarm/state";
  
  Serial.printf("Enviando actualización a Firestore (%s)...\n", updateMask.c_str());

  if (!Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.c_str(), updateMask.c_str())) {
    Serial.printf("  - Error al enviar estado: %s\n", fbdo.errorReason().c_str());
  }
}


// Función de callback para el estado del token
void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_ready) {
    Serial.println("------------------------------------");
    Serial.println("Token de Firebase listo. ¡Dispositivo Autenticado!");
    Serial.printf("UID de Dispositivo: %s\n", auth.token.uid.c_str());
    Serial.println("------------------------------------");
  } else {
    Serial.println("------------------------------------");
    Serial.println("Error de token de Firebase:");
    Serial.printf("Mensaje: %s\n", info.error.message.c_str());
    Serial.println("------------------------------------");
  }
}

// Función para sincronizar la hora (esencial para Firebase)
bool syncTime() {
  Serial.println("Sincronizando hora con servidor NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  int retries = 0;
  while (now < 8 * 3600 * 2 && retries < 30) {
      delay(500); Serial.print(".");
      now = time(nullptr);
      retries++;
  }
  if (retries >= 30) {
    Serial.println("\nFallo al sincronizar la hora después de varios intentos.");
    return false;
  }
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("\nHora sincronizada: %s", asctime(&timeinfo));
  return true;
}

// Función para conectar a la red WiFi
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Buscando redes WiFi conocidas...");
    for (int i = 0; i < numKnownNetworks; i++) {
      Serial.printf("Intentando conectar a: %s\n", knownNetworks[i].ssid);
      WiFi.begin(knownNetworks[i].ssid, knownNetworks[i].password);
      unsigned long start = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(500); Serial.print(".");
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n¡Conectado a WiFi!");
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        return;
      }
      Serial.println("\nFallo al conectar. Probando siguiente red...");
      WiFi.disconnect();
    }
    Serial.println("No se encontró ninguna red conocida. Reintentando en 10 segundos...");
    delay(10000);
  }
}
