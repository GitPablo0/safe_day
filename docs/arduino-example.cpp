/**
 * ==================================================================================
 * Código de control para un sistema de alarma PIR conectado a Firebase.
 *
 * --- ¡IMPORTANTE! INSTRUCCIONES DE CONEXIÓN ---
 * 1. Abre la aplicación web del dashboard.
 * 2. Inicia sesión y copia el "Device UID" que aparece en la tarjeta.
 * 3. Pega ese UID en la constante USER_UID de abajo.
 *
 * --- ¡IMPORTANTE! VERSIÓN DE BIBLIOTECA ---
 * Este código está diseñado para la versión 4.x.x de la biblioteca
 * "Firebase Arduino Client Library for ESP32 and ESP8266" by Mobizt.
 * ==================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// ===============================================================
// 1. CONFIGURACIÓN DE WIFI Y USUARIO
// ===============================================================
const char* WIFI_SSID = "Telecentro-416b";
const char* WIFI_PASSWORD = "ATNMFMY2MM5Q";

// --- ¡¡¡ PEGA EL UID DE TU APP AQUÍ !!! ---
#define USER_UID "eOp4B72C0wdVqf6PZDOOQGGYRdP2"


// ===============================================================
// 2. CONFIGURACIÓN DE FIREBASE
// ===============================================================
#define API_KEY "AIzaSyCMmir0FR57YVoVGQ9QCUG4oG7bUEF3w7I"
#define FIREBASE_PROJECT_ID "studio-4253330981-dcdc9"
#define DATABASE_URL "studio-4253330981-dcdc9-default-rtdb.firebaseio.com"


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

// Objetos de Firebase
FirebaseData stream;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variables de estado del sistema
bool isArmed = false;
bool presenceDetected = false;
int alarmDuration = 10; // Duración por defecto, se actualiza desde la App

// Variables para controlar la alarma
unsigned long alarmTriggeredTimestamp = 0;
bool isAlarmSounding = false;

// Declaración de funciones
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
void connectToWiFi();
void tokenStatusCallback(TokenInfo info);

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando sistema de alarma...");

  // Inicializar pines
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  pinMode(LED_AZUL_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Apagar todos los actuadores al inicio
  digitalWrite(LED_ROJO_PIN, LOW);
  digitalWrite(LED_AZUL_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  connectToWiFi();

  // Configuración de Firebase
  Serial.println("Configurando Firebase...");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Autenticación con UID personalizado en modo de prueba
  auth.user.uid = USER_UID;
  config.signer.test_mode = true;
  
  config.token_status_callback = tokenStatusCallback;

  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);

  Serial.println("Sistema de Alarma Inicializado. Esperando conexión a Firebase...");
}

void loop() {
  unsigned long currentMillis = millis();
  bool currentPresence = digitalRead(PIR_PIN) == HIGH;

  // Imprimir lecturas en tiempo real
  Serial.print("Estado: ");
  Serial.print(isArmed ? "Armado" : "Desarmado");
  Serial.print(" | Lectura PIR: ");
  Serial.println(currentPresence ? "1 (Presencia)" : "0 (Sin presencia)");

  // 1. Lógica del Sensor PIR (solo si el sistema está armado y no hay una alarma sonando)
  if (isArmed && currentPresence && !isAlarmSounding) {
      Serial.println("¡PRESENCIA DETECTADA!");
      presenceDetected = true;
      isAlarmSounding = true;
      alarmTriggeredTimestamp = currentMillis;

      // Actualizar estado en Firestore
      if(Firebase.ready()){
        String content = "{\"fields\":{\"presenceDetected\":{\"booleanValue\":true}}}";
        if (!Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", "/alarm/state", content.c_str(), "presenceDetected")) {
            Serial.printf("Error al actualizar presenceDetected: %s\n", fbdo.errorReason().c_str());
        }
      }
  }

  // 2. Lógica de la Alarma (Buzzer y LED Rojo)
  if (isAlarmSounding) {
    // Si el tiempo de alarma ha pasado o el sistema se desarma, la apagamos
    if (!isArmed || (currentMillis - alarmTriggeredTimestamp > (unsigned long)alarmDuration * 1000)) {
        isAlarmSounding = false;
        presenceDetected = false; // Resetear la detección

        if(Firebase.ready()){
          // Actualizar estado en Firestore para reflejar que la presencia ya no está activa
          String content = "{\"fields\":{\"presenceDetected\":{\"booleanValue\":false}}}";
          if (!Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", "/alarm/state", content.c_str(), "presenceDetected")) {
              Serial.printf("Error al resetear presenceDetected: %s\n", fbdo.errorReason().c_str());
          }
        }
        Serial.println("Alarma finalizada.");
    }
  }

  // 3. Control de Actuadores (LEDs y Buzzer)
  digitalWrite(LED_AZUL_PIN, isArmed); // El LED azul siempre refleja el estado 'isArmed'

  if (isAlarmSounding) {
    digitalWrite(LED_ROJO_PIN, HIGH);
    tone(BUZZER_PIN, 1000); // Emite un tono de 1kHz
  } else {
    digitalWrite(LED_ROJO_PIN, LOW);
    noTone(BUZZER_PIN); // Apaga el buzzer
  }
  
  delay(500); // Pausa para no saturar el loop
}

void streamCallback(FirebaseStream data) {
    Serial.println("¡Cambio detectado en Firebase!");
    FirebaseJson payload;
    payload.setJsonData(data.payload());
    FirebaseJsonData result;

    if(payload.get(result, "fields/isArmed/booleanValue")) {
        isArmed = result.boolValue;
        Serial.printf("  - Estado 'isArmed' actualizado a: %s\n", isArmed ? "TRUE" : "FALSE");
        // Si se desarma, se apaga cualquier alarma activa
        if (!isArmed) {
            isAlarmSounding = false;
        }
    }
    
    if(payload.get(result, "fields/alarmDuration/integerValue")) {
        alarmDuration = result.intValue;
        Serial.printf("  - 'alarmDuration' actualizada a: %d segundos\n", alarmDuration);
    }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Stream de Firebase ha expirado. Intentando reconectar...");
  }
}

void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_ready) {
    Serial.println("Token de Firebase listo. Conectando al stream...");
    // Suscribirse a cambios en el documento de estado una vez que el token está listo
    if (!Firebase.Firestore.addDocumentStream(&stream, FIREBASE_PROJECT_ID, "", "/alarm/state", streamCallback, streamTimeoutCallback)) {
      Serial.printf("Error al iniciar el stream: %s\n", stream.errorReason().c_str());
    }
  } else {
    Serial.printf("Error de token de Firebase: %s\n", info.error.message.c_str());
  }
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n¡Conectado a WiFi!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}
