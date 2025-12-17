/**
 * ==================================================================================
 * Código de control para un sistema de alarma por sonido conectado a Firebase.
 * v2 - Basado en la lógica robusta de PixelGrid.
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
// Estos valores son específicos del nuevo proyecto de Firebase.
#define API_KEY "AIzaSyALvpGn2QsEJOxuG-3xaQehba-cSdRYbVQ"
#define FIREBASE_PROJECT_ID "sistemababyalarm-3851454-f8120"
#define DATABASE_URL "https://sistemababyalarm-3851454-f8120-default-rtdb.firebaseio.com"

// ===============================================
// 3. PINES Y SENSORES
// ===============================================
#define SOUND_PIN 27      // Pin para el sensor de sonido digital (DO)
#define LED_ROJO_PIN 4    // Pin para el LED rojo (detección de sonido)
#define LED_AZUL_PIN 2    // Pin para el LED azul (sistema armado)
#define BUZZER_PIN 22     // Pin para el buzzer (opcional, para alarma sonora)

// ===============================================
// 4. OBJETOS Y VARIABLES DE ESTADO
// ===============================================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variables de estado del sistema
bool isArmed = false;
bool soundDetected = false;
unsigned long lastSoundDetectedTime = 0;
const unsigned long SOUND_COOLDOWN = 3000; // 3 segundos de espera entre detecciones

// Estado anterior del sensor para detectar flancos (evita lecturas continuas)
int lastSoundSensorState = HIGH; // Los sensores digitales suelen estar en HIGH en reposo

unsigned long lastFirestoreRead = 0;
const long firestoreReadInterval = 3000; // Leer estado desde Firestore cada 3 segundos

// ===============================================
// 5. DECLARACIÓN DE FUNCIONES
// ===============================================
void connectToWiFi();
bool syncTime();
void tokenStatusCallback(TokenInfo info);
void readAlarmStateFromFirestore();
void sendSoundDetectionToFirestore(bool detected);
String getJsonField(String& payload, const char* fieldName);

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando Sistema de Alarma por Sonido v2...");

  pinMode(SOUND_PIN, INPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  pinMode(LED_AZUL_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_ROJO_PIN, LOW);
  digitalWrite(LED_AZUL_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

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

  // No hacer nada hasta que Firebase esté listo y el UID esté configurado.
  if (!Firebase.ready() || strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
    if (strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
        Serial.println("UID de usuario no configurado. Pega el UID en el código.");
        delay(5000);
    }
    return;
  }

  // 1. Leer estado de la alarma desde Firestore periódicamente
  if (currentMillis - lastFirestoreRead > firestoreReadInterval) {
    lastFirestoreRead = currentMillis;
    readAlarmStateFromFirestore();
  }

  // 2. Lógica de detección de sonido (basada en flanco descendente)
  int currentSoundState = digitalRead(SOUND_PIN);

  // Un sonido es detectado cuando el pin pasa de HIGH a LOW (lo más común)
  if (isArmed && currentSoundState == LOW && lastSoundSensorState == HIGH) {
    if (currentMillis - lastSoundDetectedTime > SOUND_COOLDOWN) {
        Serial.println("¡SONIDO DETECTADO!");
        soundDetected = true;
        sendSoundDetectionToFirestore(true);
        lastSoundDetectedTime = currentMillis;
        
        // Activa el LED localmente de inmediato
        digitalWrite(LED_ROJO_PIN, HIGH);
        // Opcional: activar buzzer
        // digitalWrite(BUZZER_PIN, HIGH);
    }
  }
  
  // Actualizar el estado anterior del sensor
  lastSoundSensorState = currentSoundState;


  // 3. Control de actuadores visuales
  digitalWrite(LED_AZUL_PIN, isArmed);
}

// Función para leer el estado de la alarma desde Firestore
void readAlarmStateFromFirestore() {
  String documentPath = "users/" + String(USER_UID) + "/soundAlarm/state";
  
  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
    if (fbdo.httpCode() == 200) {
      FirebaseJson json;
      json.setJsonData(fbdo.payload());
      FirebaseJsonData result;

      // Parsear el campo 'isArmed'
      if (json.get(result, "fields/isArmed/booleanValue")) {
        if(result.success) {
            bool newArmedState = result.to<bool>();
            if (isArmed != newArmedState) {
                isArmed = newArmedState;
                Serial.printf("Estado 'isArmed' actualizado desde Firestore a: %s\n", isArmed ? "ARMADO" : "DESARMADO");
                
                // Si se desarma, resetea la detección de sonido local y remotamente
                if (!isArmed) {
                    soundDetected = false;
                    sendSoundDetectionToFirestore(false);
                    digitalWrite(LED_ROJO_PIN, LOW);
                    digitalWrite(BUZZER_PIN, LOW);
                }
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

// Función para enviar el estado de detección a Firestore
void sendSoundDetectionToFirestore(bool detected) {
  String documentPath = "users/" + String(USER_UID) + "/soundAlarm/state";
  String content = "{\"fields\":{\"soundDetected\":{\"booleanValue\":" + String(detected ? "true" : "false") + "}}}";
  
  Serial.printf("Enviando 'soundDetected: %s' a Firestore...\n", detected ? "true" : "false");

  // El "updateMask" (último parámetro) le dice a Firestore que solo actualice el campo "soundDetected".
  if (!Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.c_str(), "soundDetected")) {
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
