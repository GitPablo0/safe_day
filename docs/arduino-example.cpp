/**
 * ==================================================================================
 * Código de control para un sistema de alarma por sonido conectado a Firebase.
 *
 * Puntos Clave:
 * 1. UID de Usuario Manual: El dispositivo se vincula a una cuenta de usuario específica
 *    en la app web. Debes copiar el UID y pegarlo en la variable USER_UID.
 * 2. Estructura de datos por usuario: /users/{UID}/soundAlarm/state
 * 3. Sincronización de Hora NTP: Crítico para la validación de tokens de Firebase.
 * 4. Loop no bloqueante: Usa millis() para operaciones temporizadas.
 * 5. Detección por Nivel Lógico: La mayoría de sensores de sonido digitales
 *    emiten un nivel BAJO (LOW) cuando detectan un sonido que supera su umbral.
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
// 2. CONFIGURACIÓN DE FIREBASE
// ===============================================================
#define API_KEY "AIzaSyD26vBFuUd45oUiio76gLNAz6Mh3062Muw"
#define FIREBASE_PROJECT_ID "sistemaalarma-11019109-94c6b"
#define DATABASE_URL "https://sistemaalarma-11019109-94c6b-default-rtdb.firebaseio.com/"

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
const unsigned long SOUND_COOLDOWN = 2000; // 2 segundos de espera entre detecciones

// Estado anterior del sensor para detectar flancos
int lastSoundSensorState = HIGH;

unsigned long lastFirestoreRead = 0;
const long firestoreReadInterval = 5000; // Leer estado desde Firestore cada 5 segundos

// ===============================================
// 5. DECLARACIÓN DE FUNCIONES
// ===============================================
void connectToWiFi();
void tokenStatusCallback(TokenInfo info);
void readAlarmStateFromFirestore();
void sendSoundDetectionToFirestore(bool detected);
String getJsonField(String& payload, const char* fieldName);

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando sistema de alarma por sonido...");

  pinMode(SOUND_PIN, INPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  pinMode(LED_AZUL_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_ROJO_PIN, LOW);
  digitalWrite(LED_AZUL_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  connectToWiFi();

  // --- SINCRONIZACIÓN DE HORA (NTP) ---
  Serial.println("Sincronizando hora con servidor NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
      delay(500);
      Serial.print(".");
      now = time(nullptr);
  }
  Serial.println("\nHora sincronizada correctamente.");

  // --- AUTENTICACIÓN ---
  Serial.println("Configurando Firebase...");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  auth.user.email = "pabloportillo12345@gmail.com";
  auth.user.password = "admin123";

  config.token_status_callback = tokenStatusCallback;
  
  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);

  Serial.println("Sistema de Alarma por Sonido Inicializado. Esperando conexión a Firebase...");
}

void loop() {
  unsigned long currentMillis = millis();

  if (!Firebase.ready() || strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
    if (strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
        Serial.println("UID de usuario no configurado. Pegue el UID en el código.");
        delay(5000);
    }
    return;
  }

  // 1. Leer estado de la alarma desde Firestore periódicamente
  if (currentMillis - lastFirestoreRead > firestoreReadInterval) {
    lastFirestoreRead = currentMillis;
    readAlarmStateFromFirestore();
  }

  // 2. Lógica de detección de sonido
  int currentSoundState = digitalRead(SOUND_PIN);

  if (isArmed && currentSoundState == LOW && lastSoundSensorState == HIGH) {
    if (currentMillis - lastSoundDetectedTime > SOUND_COOLDOWN) {
        Serial.println("¡SONIDO DETECTADO!");
        soundDetected = true;
        sendSoundDetectionToFirestore(true);
        lastSoundDetectedTime = currentMillis;
        
        // Activa el buzzer y el LED localmente de inmediato
        digitalWrite(LED_ROJO_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
    }
  }

  // 3. Resetear la detección si ya no se detecta sonido (opcional, depende del sensor)
  // O si se desarma la alarma
  if ((isArmed && currentSoundState == HIGH && soundDetected) || (!isArmed && soundDetected)) {
      Serial.println("Condiciones de sonido reseteadas.");
      soundDetected = false;
      sendSoundDetectionToFirestore(false);
      digitalWrite(LED_ROJO_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
  }
  
  lastSoundSensorState = currentSoundState;


  // 4. Control de actuadores visuales
  digitalWrite(LED_AZUL_PIN, isArmed);
}

String getJsonField(String& payload, const char* fieldName) {
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

// Función para leer el estado de la alarma desde Firestore
void readAlarmStateFromFirestore() {
  String documentPath = "users/" + String(USER_UID) + "/soundAlarm/state";
  Serial.println("Leyendo estado desde Firestore: " + documentPath);

  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
    if (fbdo.httpCode() == 200) {
      String payload = fbdo.payload();
      FirebaseJson json;
      
      String fieldsJson = getJsonField(payload, "fields");
      json.setJsonData(fieldsJson);
      
      FirebaseJsonData result;

      if (json.get(result, "isArmed/booleanValue")) {
        bool newArmedState = result.to<bool>();
        if (isArmed != newArmedState) {
            isArmed = newArmedState;
            Serial.printf("  - Estado 'isArmed' actualizado a: %s\n", isArmed ? "ARMADO" : "DESARMADO");
            if (!isArmed) { // Si se desarma, resetea todo
                soundDetected = false;
                sendSoundDetectionToFirestore(false);
                digitalWrite(LED_ROJO_PIN, LOW);
                digitalWrite(BUZZER_PIN, LOW);
            }
        }
      }
    } else {
      Serial.printf("Error al leer el documento. HTTP Code: %d, Razón: %s\n", fbdo.httpCode(), fbdo.errorReason().c_str());
    }
  } else {
    Serial.println(fbdo.errorReason().c_str());
  }
}

// Función para enviar el estado de detección a Firestore
void sendSoundDetectionToFirestore(bool detected) {
  String documentPath = "users/" + String(USER_UID) + "/soundAlarm/state";
  String content = "{\"fields\":{\"soundDetected\":{\"booleanValue\":" + String(detected ? "true" : "false") + "}}}";
  
  Serial.printf("Enviando 'soundDetected: %s' a Firestore...\n", detected ? "true" : "false");

  if (!Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.c_str(), "soundDetected")) {
    Serial.printf("  - Error al enviar estado: %s\n", fbdo.errorReason().c_str());
  }
}

// Función de callback para el estado del token
void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_ready) {
    Serial.println("Token de Firebase listo. ¡Conectado y autenticado!");
    Serial.printf("UID del dispositivo: %s\n", auth.token.uid.c_str());
  } else {
    Serial.println("------------------------------------");
    Serial.println("Error de token de Firebase:");
    Serial.println(info.error.message.c_str());
    Serial.println("------------------------------------");
  }
}

// Función para conectar a la red WiFi
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Buscando redes WiFi conocidas...");
    for (int i = 0; i < numKnownNetworks; i++) {
      Serial.print("Intentando conectar a: ");
      Serial.println(knownNetworks[i].ssid);
      
      WiFi.begin(knownNetworks[i].ssid, knownNetworks[i].password);
      
      unsigned long startAttemptTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
        Serial.print(".");
        delay(500);
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n¡Conectado a WiFi!");
        Serial.print("Red: ");
        Serial.println(WiFi.SSID());
        Serial.print("Dirección IP: ");
        Serial.println(WiFi.localIP());
        return;
      } else {
        Serial.println("\nFallo al conectar. Probando siguiente red...");
        WiFi.disconnect();
      }
    }
    Serial.println("No se encontró ninguna red conocida. Reintentando en 10 segundos...");
    delay(10000);
  }
}
