
/**
 * ==================================================================================
 * SafeDay v1.1 - Sistema de apoyo Alzheimer con ESP32 y DFPlayer Mini.
 * Hardware: ESP32, DFPlayer Mini, Pulsador (Pin 14), LED (Pin 2), MicroSD.
 * ==================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// 1. CONFIGURACIÓN USUARIO (Copia el UID desde la WebApp)
#define USER_UID "TU_USER_UID_AQUI"

// 2. CONFIGURACIÓN WIFI
const char* ssid = "TU_WIFI_NAME";
const char* password = "TU_WIFI_PASSWORD";

// 3. PINES
#define BTN_STOP_PIN 14
#define LED_INDICATOR 2
#define DF_RX 16
#define DF_TX 17

// 4. OBJETOS
SoftwareSerial softSerial(DF_RX, DF_TX);
DFRobotDFPlayerMini myDFPlayer;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// 5. VARIABLES DE ESTADO
bool isAlarmActive = false;
unsigned long lastCheck = 0;
const long checkInterval = 15000; // Revisar rutina cada 15s
String lastTriggeredTime = "";

void syncTime() {
  const char* ntpServer = "pool.ntp.org";
  const long  gmtOffset_sec = -10800; // Ajustar según tu zona horaria (ej: -3h = -10800s)
  const int   daylightOffset_sec = 0;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Sincronizando hora...");
}

void setup() {
  Serial.begin(115200);
  softSerial.begin(9600);
  
  pinMode(BTN_STOP_PIN, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);
  
  if (!myDFPlayer.begin(softSerial)) {
    Serial.println("Error: DFPlayer no detectado. Revisa conexiones y MicroSD.");
  } else {
    Serial.println("DFPlayer Mini Online.");
    myDFPlayer.volume(20);
  }

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\nWiFi Conectado.");
  
  syncTime();
  
  // Configuración Firebase (Reemplazar con tus credenciales de Proyecto)
  config.api_key = "TU_API_KEY_DE_FIREBASE";
  config.database_url = "TU_DATABASE_URL"; 
  
  // Credenciales para el dispositivo (Usar el usuario creado en la app)
  auth.user.email = "tu@email.com";
  auth.user.password = "tu_password";
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void triggerAlarm(int track) {
  if (isAlarmActive) return;
  isAlarmActive = true;
  myDFPlayer.play(track);
  Serial.printf("¡ALERTA ACTIVADA! Reproduciendo pista %d\n", track);
}

void checkRoutine() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Error al obtener la hora");
    return;
  }

  char currentTime[6];
  sprintf(currentTime, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  String nowStr = String(currentTime);

  if (nowStr == lastTriggeredTime) return;

  // Consulta Firestore: users/{USER_UID}/routine
  // Implementar lógica de lectura de colección según la librería
  // Firebase.Firestore.getDocument(&fbdo, ...);
  
  // Simulacro de detección:
  // if (hayTareaProgramadaPara(nowStr)) {
  //    triggerAlarm(tarea.trackNumber);
  //    lastTriggeredTime = nowStr;
  // }
}

void loop() {
  if (isAlarmActive) {
    // Parpadeo de LED mientras la alarma suena
    digitalWrite(LED_INDICATOR, (millis() / 500) % 2);
    
    if (digitalRead(BTN_STOP_PIN) == LOW) {
      Serial.println("Botón presionado: Alarma detenida por el paciente.");
      myDFPlayer.stop();
      digitalWrite(LED_INDICATOR, LOW);
      isAlarmActive = false;
      delay(1000); // Debounce
    }
  }

  if (millis() - lastCheck > checkInterval) {
    lastCheck = millis();
    checkRoutine();
  }
}
