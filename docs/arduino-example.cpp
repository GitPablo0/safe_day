/**
 * ==================================================================================
 * Código de control para un sistema de alarma PIR conectado a Firebase RTDB.
 * Versión corregida para usar autenticación anónima.
 *
 * --- ¡IMPORTANTE! INSTRUCCIONES DE CONEXIÓN ---
 * 1. Abre la aplicación web del dashboard.
 * 2. Inicia sesión y copia el "Device UID" que aparece en la tarjeta.
 * 3. Pega ese UID en la constante USER_UID de abajo.
 * ==================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// ===============================================================
// 1. CONFIGURACIÓN DE WIFI Y USUARIO
// ===============================================================
// ¡¡¡ PEGA TU UID DE USUARIO AQUÍ !!!
#define USER_UID "PASTE_YOUR_USER_UID_HERE"

// Estructura para almacenar credenciales WiFi
struct WiFiNetwork {
  const char* ssid;
  const char* password;
};

// Lista de redes WiFi conocidas. Añade las tuyas aquí.
WiFiNetwork knownNetworks[] = {
  {"Telecentro-416b", "ATNMFMY2MM5Q"},
  {"Pablooo13", "brenda123"},
  {"Estudiantes", "Escuelas_2025"}
  // Puedes añadir más redes si lo necesitas
};
const int numKnownNetworks = sizeof(knownNetworks) / sizeof(knownNetworks[0]);


// ===============================================================
// 2. CONFIGURACIÓN DE FIREBASE
// ===============================================================
#define API_KEY "AIzaSyD26vBFuUd45oUiio76gLNAz6Mh3062Muw"
#define FIREBASE_PROJECT_ID "sistemaalarma-11019109-94c6b"
#define DATABASE_URL "https://sistemaalarma-11019109-94c6b-default-rtdb.firebaseio.com"


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
FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

// Variables de estado del sistema
bool isArmed = false;
bool presenceDetected = false;
unsigned long lastBuzzerToggleTime = 0;
const long buzzerToggleInterval = 250; // Intervalo para el parpadeo del buzzer

// Declaración de funciones
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
  
  // Asignar la función de callback para el estado del token
  config.token_status_callback = tokenStatusCallback;

  Firebase.reconnectWiFi(true);
  
  // Iniciar Firebase con autenticación anónima
  Firebase.begin(&config, &auth);
  
  Serial.println("Solicitando autenticación anónima...");

  Serial.println("Sistema de Alarma Inicializado. Esperando conexión a Firebase...");
}

void loop() {
  // 1. Leer datos del stream de Firebase si está listo
  if (Firebase.ready() && stream.httpConnected()) {
      if (Firebase.RTDB.readStream(&stream)) {
        if (stream.streamTimeout()) {
            Serial.println("Stream de Firebase ha expirado. Intentando reconectar...");
        } else if (stream.dataTypeEnum() == fb_esp_rtdb_data_type_json) {
            FirebaseJson *json = stream.to<FirebaseJson *>();
            FirebaseJsonData result;

            // Buscar el campo "isArmed"
            if (json->get(result, "isArmed")) {
                if (result.typeNum == FirebaseJson::JSON_BOOL) {
                    bool newArmedState = result.to<bool>();
                    if (isArmed != newArmedState) {
                       isArmed = newArmedState;
                       Serial.printf("  - Estado 'isArmed' actualizado a: %s\n", isArmed ? "TRUE" : "FALSE");
                    }
                    
                    // Si se desarma el sistema, reseteamos la detección de presencia
                    if (!isArmed && presenceDetected) {
                        presenceDetected = false;
                        Serial.println("  - Sistema desarmado, reseteando presencia.");
                        if(Firebase.ready()){
                           Firebase.RTDB.setBool(&fbdo, "/alarm/state/presenceDetected", false);
                        }
                    }
                }
            }

            // Escuchar también por si la app resetea la presencia manualmente
            if (json->get(result, "presenceDetected")) {
                 if(result.typeNum == FirebaseJson::JSON_BOOL) {
                    bool newPresenceState = result.to<bool>();
                    if (presenceDetected != newPresenceState) {
                        presenceDetected = newPresenceState;
                        Serial.printf("  - Estado 'presenceDetected' actualizado remotamente a: %s\n", presenceDetected ? "TRUE" : "FALSE");
                    }
                }
            }
        }
      }
  }

  // 2. Lectura del sensor PIR
  bool currentPresence = digitalRead(PIR_PIN) == HIGH;

  // Si el sistema está armado y se detecta presencia por primera vez
  if (isArmed && currentPresence && !presenceDetected) {
      Serial.println("¡PRESENCIA DETECTADA!");
      presenceDetected = true;
      
      // Enviar el estado a Firebase Realtime Database
      if (Firebase.ready()) {
          if (Firebase.RTDB.setBool(&fbdo, "/alarm/state/presenceDetected", true)) {
            Serial.println("  - Estado 'presenceDetected' enviado a Firebase.");
          } else {
            Serial.printf("  - Error al enviar estado a Firebase: %s\n", fbdo.errorReason().c_str());
          }
      }
  }

  // 3. Control de actuadores
  digitalWrite(LED_AZUL_PIN, isArmed); // LED azul encendido si el sistema está armado

  if (presenceDetected) {
    // Si hay presencia, el LED rojo se enciende fijo
    digitalWrite(LED_ROJO_PIN, HIGH);
    // Y el buzzer emite un sonido intermitente
    unsigned long currentMillis = millis();
    if (currentMillis - lastBuzzerToggleTime > buzzerToggleInterval) {
      lastBuzzerToggleTime = currentMillis;
      digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN)); // Alterna el estado del buzzer
    }
  } else {
    // Si no hay presencia, apagar LED rojo y buzzer
    digitalWrite(LED_ROJO_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
  
  delay(100); // Pequeña pausa para estabilizar el loop
}

// Esta función se llama si el stream de Firebase expira
void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Stream de Firebase ha expirado. Intentando reconectar...");
  }
}

// Esta función se llama para informar sobre el estado del token de autenticación
void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_ready) {
    // Usar auth.token.uid.c_str() para la v4.x.x
    Serial.printf("Token de Firebase listo. UID de Auth: %s\n", auth.token.uid.c_str());
    Serial.println("Conectando al stream de Realtime Database...");
    
    // Iniciar el stream para escuchar cambios en el documento /alarm/state
    if (!Firebase.RTDB.beginStream(&stream, "/alarm/state")) {
      Serial.printf("Error al iniciar el stream de RTDB: %s\n", stream.errorReason().c_str());
    }
    
    Firebase.RTDB.setStreamCallback(&stream, streamTimeoutCallback);

  } else {
    Serial.println("------------------------------------");
    Serial.printf("Error de token de Firebase: %s\n", info.error.message.c_str());
    Serial.println("------------------------------------");
  }
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Buscando redes WiFi conocidas...");
    for (int i = 0; i < numKnownNetworks; i++) {
      Serial.print("Intentando conectar a: ");
      Serial.println(knownNetworks[i].ssid);
      
      WiFi.begin(knownNetworks[i].ssid, knownNetworks[i].password);
      
      unsigned long startAttemptTime = millis();
      // Esperar hasta 15 segundos por cada red
      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
        Serial.print(".");
        delay(500);
      }
      
      // Si se conecta, salir del bucle
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n¡Conectado a WiFi!");
        Serial.print("Red: ");
        Serial.println(WiFi.SSID());
        Serial.print("Dirección IP: ");
        Serial.println(WiFi.localIP());
        return; // Salir de la función
      } else {
        Serial.println("\nFallo al conectar. Probando siguiente red...");
        WiFi.disconnect();
      }
    }
    Serial.println("No se encontró ninguna red conocida. Reintentando en 10 segundos...");
    delay(10000);
  }
}
