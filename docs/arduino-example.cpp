
/**
 * ==================================================================================
 * SafeDay v1 - Sistema de apoyo Alzheimer con ESP32 y DFPlayer Mini.
 * hardware: ESP32, DFPlayer Mini, Pulsador (Pin 14), LED (Pin 2).
 * ==================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// 1. CONFIGURACIÓN USUARIO (Pega tu UID aquí)
#define USER_UID "PASTE_YOUR_USER_UID_HERE"

// 2. CONFIGURACIÓN WIFI
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";

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
const long checkInterval = 10000; // Revisar rutina cada 10s
String lastTriggeredTime = "";

void setup() {
  Serial.begin(115200);
  softSerial.begin(9600);
  
  pinMode(BTN_STOP_PIN, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  
  if (!myDFPlayer.begin(softSerial)) {
    Serial.println("Error: DFPlayer no detectado.");
  }
  myDFPlayer.volume(20);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  configTime(0, 0, "pool.ntp.org"); // Sincronizar hora
  
  config.api_key = "TU_API_KEY";
  config.database_url = "TU_RTDB_URL"; // Si usas RTDB o Firestore
  auth.user.email = "dispositivo@safeday.com";
  auth.user.password = "safeday123";
  
  Firebase.begin(&config, &auth);
}

void loop() {
  if (isAlarmActive) {
    digitalWrite(LED_INDICATOR, (millis() / 500) % 2); // Parpadeo
    if (digitalRead(BTN_STOP_PIN) == LOW) {
      Serial.println("Botón presionado: Alarma detenida.");
      myDFPlayer.stop();
      digitalWrite(LED_INDICATOR, LOW);
      isAlarmActive = false;
    }
  }

  if (millis() - lastCheck > checkInterval) {
    lastCheck = millis();
    checkRoutine();
  }
}

void checkRoutine() {
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  char currentTime[6];
  sprintf(currentTime, "%02d:%02d", p_tm->tm_hour, p_tm->tm_min);
  
  if (String(currentTime) == lastTriggeredTime) return;

  // Consultar Firestore: users/{USER_UID}/routine
  // Nota: Implementar lectura de colección con Firebase.Firestore.getDocument
  // Si encontramos una tarea con la hora actual:
  // triggerAlarm(trackNumber);
  // lastTriggeredTime = String(currentTime);
}

void triggerAlarm(int track) {
  isAlarmActive = true;
  myDFPlayer.play(track);
  Serial.printf("Disparando alarma: Pista %d\n", track);
}
