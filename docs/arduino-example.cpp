/**
 * ==================================================================================
 * Código de control para un sistema de alarma PIR conectado a Firebase.
 * Versión corregida siguiendo la guía y las mejores prácticas del proyecto NeuroSense.
 *
 * Puntos Clave Implementados:
 * 1. UID de Usuario Manual: El dispositivo se vincula a una cuenta de usuario específica.
 *    --> ¡IMPORTANTE! Debes copiar el UID de la app y pegarlo en la variable USER_UID.
 * 2. Estructura de datos por usuario: Todos los datos se guardan bajo /users/{UID}/...
 * 3. Sincronización de Hora NTP: Crítico para la validación de tokens de Firebase.
 * 4. Autenticación por Email/Contraseña: El dispositivo se autentica como un usuario único.
 * 5. Loop no bloqueante: Usa millis() para operaciones temporizadas.
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
// La Database URL es para la autenticación, que usa RTDB por debajo.
#define DATABASE_URL "https://sistemaalarma-11019109-94c6b-default-rtdb.firebaseio.com/"

// ===============================================
// 3. PINES Y SENSORES
// ===============================================
#define PIR_PIN 23       // Pin para el sensor de movimiento PIR
#define LED_ROJO_PIN 16  // Pin para el LED rojo (detección de presencia)
#define LED_AZUL_PIN 15  // Pin para el LED azul (sistema armado)
#define BUZZER_PIN 22    // Pin para el buzzer

// ===============================================
// 4. OBJETOS Y VARIABLES DE ESTADO
// ===============================================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variables de estado del sistema
bool isArmed = false; // Este estado lo leeremos desde Firestore
bool presenceDetected = false;
unsigned long lastBuzzerToggleTime = 0;
const long buzzerToggleInterval = 250;

unsigned long lastFirestoreRead = 0;
const long firestoreReadInterval = 5000; // Leer estado desde Firestore cada 5 segundos

unsigned long lastPresenceSend = 0;
const long presenceSendInterval = 2000; // Enviar detección de presencia cada 2 segundos si está activa

// ===============================================
// 5. DECLARACIÓN DE FUNCIONES
// ===============================================
void connectToWiFi();
void tokenStatusCallback(TokenInfo info);
void readAlarmStateFromFirestore();
void sendPresenceToFirestore(bool detected);
String getJsonField(String& payload, const char* fieldName);

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando sistema de alarma...");

  pinMode(PIR_PIN, INPUT);
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
  
  // ¡IMPORTANTE! Credenciales del dispositivo para autenticación.
  auth.user.email = "pabloportillo12345@gmail.com";
  auth.user.password = "admin123";

  config.token_status_callback = tokenStatusCallback;
  
  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);

  Serial.println("Sistema de Alarma Inicializado. Esperando conexión a Firebase...");
}

void loop() {
  unsigned long currentMillis = millis();

  // No hacer nada hasta que Firebase esté listo y tengamos un UID
  if (!Firebase.ready() || strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
    if (strcmp(USER_UID, "PASTE_YOUR_USER_UID_HERE") == 0) {
        Serial.println("UID de usuario no configurado. Pegue el UID en el código.");
        delay(5000);
    }
    return;
  }

  // 1. Leer estado desde Firestore periódicamente
  if (currentMillis - lastFirestoreRead > firestoreReadInterval) {
    lastFirestoreRead = currentMillis;
    readAlarmStateFromFirestore();
  }

  // 2. Lógica de detección de presencia
  if (isArmed && digitalRead(PIR_PIN) == HIGH) {
    if (!presenceDetected) {
      Serial.println("¡PRESENCIA DETECTADA!");
      presenceDetected = true;
      sendPresenceToFirestore(true); // Enviar inmediatamente al detectar
      lastPresenceSend = currentMillis;
    }
  }

  // 3. Enviar estado de presencia periódicamente si sigue detectada
  if (presenceDetected && (currentMillis - lastPresenceSend > presenceSendInterval)) {
    lastPresenceSend = currentMillis;
    sendPresenceToFirestore(true);
  }
  
  // 4. Si se desarma, resetear la presencia.
  if (!isArmed && presenceDetected) {
    presenceDetected = false;
    Serial.println("Sistema desarmado, reseteando presencia.");
    sendPresenceToFirestore(false);
  }

  // 5. Control de actuadores locales
  digitalWrite(LED_AZUL_PIN, isArmed);

  if (presenceDetected) {
    digitalWrite(LED_ROJO_PIN, HIGH);
    if (currentMillis - lastBuzzerToggleTime > buzzerToggleInterval) {
      lastBuzzerToggleTime = currentMillis;
      digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN));
    }
  } else {
    digitalWrite(LED_ROJO_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
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
  String documentPath = "users/" + String(USER_UID) + "/alarm/state";
  Serial.println("Leyendo estado desde Firestore: " + documentPath);

  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
    if (fbdo.httpCode() == 200) {
      String payload = fbdo.payload();
      FirebaseJson json;
      
      // Extraemos el objeto 'fields' completo
      String fieldsJson = getJsonField(payload, "fields");
      json.setJsonData(fieldsJson);
      
      FirebaseJsonData result;

      if (json.get(result, "isArmed/booleanValue")) {
        isArmed = result.to<bool>();
        Serial.printf("  - Estado 'isArmed' actualizado a: %s\n", isArmed ? "ARMADO" : "DESARMADO");
      }
    } else {
      Serial.printf("Error al leer el documento. HTTP Code: %d, Razón: %s\n", fbdo.httpCode(), fbdo.errorReason().c_str());
    }
  } else {
    Serial.println(fbdo.errorReason().c_str());
  }
}

// Función para enviar el estado de presencia a Firestore
void sendPresenceToFirestore(bool detected) {
  String documentPath = "users/" + String(USER_UID) + "/alarm/state";
  
  // El payload debe especificar el tipo de dato para Firestore
  String content = "{\"fields\":{\"presenceDetected\":{\"booleanValue\":" + String(detected ? "true" : "false") + "}}}";
  
  Serial.printf("Enviando 'presenceDetected: %s' a Firestore...\n", detected ? "true" : "false");

  // El updateMask "presenceDetected" le dice a Firestore que solo actualice ese campo.
  if (!Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.c_str(), "presenceDetected")) {
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
