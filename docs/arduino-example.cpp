/**
 * ==================================================================================
 * Código de control para un sistema de alarma PIR conectado a Firebase.
 * Adaptado para funcionar con la aplicación web y la biblioteca Firebase-ESP-Client v4.x.x
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
#include "time.h"

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
// 2. CONFIGURACIÓN DE FIREBASE (CORREGIDA)
// ===============================================================
#define API_KEY "AIzaSyD26vBFuUd45oUiio76gLNAz6Mh3062Muw"
#define FIREBASE_PROJECT_ID "sistemaalarma-11019109-94c6b"
#define DATABASE_URL "sistemaalarma-11019109-94c6b-default-rtdb.firebaseio.com"

// Objetos de Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;


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

// Variables de estado del sistema
bool isArmed = false;
bool presenceDetected = false;
unsigned long lastBuzzerToggleTime = 0;
const long buzzerToggleInterval = 250; // Intervalo para el parpadeo del buzzer

// Variables para el sondeo (polling)
unsigned long lastStateFetchMillis = 0;
const long stateFetchInterval = 2000; // Consultar a Firebase cada 2 segundos

// Declaración de funciones
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

  // Sincronizar la hora (crucial para la conexión SSL/TLS con Firebase)
  Serial.println("Sincronizando hora con servidor NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
      delay(500);
      Serial.print(".");
      now = time(nullptr);
  }
  Serial.println("\nHora sincronizada.");


  // Configuración de Firebase (replicando el método funcional)
  Serial.println("Configurando Firebase...");
  config.api_key = API_KEY;

  // Usar un email y password de placeholder como en el ejemplo funcional
  auth.user.email = "esp32@alumnosuade.com";
  auth.user.password = "password";

  // Asignar el UID que se usará para la autenticación de prueba
  config.signer.test_mode = true;
  
  config.token_status_callback = tokenStatusCallback;

  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);

  Serial.println("Sistema de Alarma Inicializado. Esperando conexión a Firebase...");
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. SONDEO A FIREBASE PARA OBTENER EL ESTADO ---
  // Si Firebase está listo y ha pasado el intervalo de sondeo...
  if (Firebase.ready() && (currentMillis - lastStateFetchMillis > stateFetchInterval)) {
    lastStateFetchMillis = currentMillis;

    String documentPath = "/alarm/state";
    
    // Intentar obtener el documento de Firestore
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
        // Verificar si la petición HTTP fue exitosa
        if(fbdo.httpCode() == 200) {
            String payload = fbdo.payload();
            
            // Procesar el JSON de respuesta de Firestore
            FirebaseJson json;
            // La respuesta de Firestore anida los campos en un objeto "fields"
            String fieldsPath = "/fields";
            json.setJsonData(payload);
            FirebaseJsonData result;

            // Obtener el valor de 'isArmed'
            if (json.get(result, fieldsPath + "/isArmed/booleanValue")) {
                bool armedStateFromFirebase = result.to<bool>();
                // Actualizar solo si el estado ha cambiado
                if (isArmed != armedStateFromFirebase) {
                    isArmed = armedStateFromFirebase;
                    Serial.printf("Estado 'isArmed' actualizado desde Firebase a: %s\n", isArmed ? "TRUE" : "FALSE");
                    // Si se desarma desde la app, reseteamos la detección de presencia
                    if (!isArmed && presenceDetected) {
                        presenceDetected = false;
                        // Notificar a Firebase que la presencia ha sido reseteada
                        Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), "{\"fields\":{\"presenceDetected\":{\"booleanValue\":false}}}", "presenceDetected");
                    }
                }
            }

            // Obtener el valor de 'presenceDetected' (por si se resetea desde la app)
            if (json.get(result, fieldsPath + "/presenceDetected/booleanValue")) {
                bool presenceStateFromFirebase = result.to<bool>();
                if (presenceDetected != presenceStateFromFirebase) {
                    presenceDetected = presenceStateFromFirebase;
                     Serial.printf("Estado 'presenceDetected' actualizado desde Firebase a: %s\n", presenceDetected ? "TRUE" : "FALSE");
                }
            }
        }
    }
  } else {
    // ----> AÑADIDO PARA DIAGNÓSTICO <----
    // Si Firebase no está listo, imprime un punto para saber que el loop sigue corriendo.
    Serial.print(".");
    delay(100);
  }


  // --- 2. LECTURA DEL SENSOR PIR ---
  bool currentPresence = digitalRead(PIR_PIN) == HIGH;

  // Si el sistema está armado y se detecta presencia por primera vez
  if (isArmed && currentPresence && !presenceDetected) {
      Serial.println("¡PRESENCIA DETECTADA!");
      presenceDetected = true;
      
      // Enviar el estado a Firebase Firestore
      if (Firebase.ready()) {
        String documentPath = "/alarm/state";
        // Contenido del parche para actualizar solo el campo 'presenceDetected'
        String patchContent = "{\"fields\":{\"presenceDetected\":{\"booleanValue\":true}}}";
        if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), patchContent.c_str(), "presenceDetected")) {
          Serial.println("  - Estado 'presenceDetected' enviado a Firebase.");
        } else {
          Serial.printf("  - Error al enviar estado a Firebase: %s\n", fbdo.errorReason().c_str());
        }
      }
  }

  // --- 3. CONTROL DE ACTUADORES ---
  digitalWrite(LED_AZUL_PIN, isArmed); // LED azul encendido si el sistema está armado

  if (presenceDetected) {
    // Si hay presencia, el LED rojo se enciende fijo
    digitalWrite(LED_ROJO_PIN, HIGH);
    // Y el buzzer emite un sonido intermitente
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

// Esta función se llama para informar sobre el estado del token de autenticación
void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_ready) {
    Serial.println("\n------------------------------------");
    Serial.println("Token de Firebase listo. ¡Conectado!");
    Serial.printf("UID del Dispositivo: %s\n", auth.token.uid.c_str());
    Serial.println("------------------------------------");
  } else {
    Serial.println("\n------------------------------------");
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
