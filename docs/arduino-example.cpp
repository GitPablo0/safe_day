
/**
 * ==================================================================================
 * SafeDay v1.2 - Sistema de apoyo Alzheimer (ESP32 + DFPlayer Mini)
 * Hardware: ESP32, DFPlayer Mini, Pulsador (Pin 14), LED (Pin 2), Parlante.
 * ==================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// 1. CONFIGURACIÓN - REEMPLAZAR CON TUS DATOS
#define WIFI_SSID "TU_WIFI_NAME"
#define WIFI_PASSWORD "TU_WIFI_PASSWORD"

#define API_KEY "AIzaSyAdUgSVPEpdoLF15UleY65qm3PtjC82h-8"
#define FIREBASE_PROJECT_ID "safeday-29789405-ff4df"
#define USER_UID "TU_USER_UID_AQUI" // Copia esto desde el Dashboard de la App

// 2. PINES
#define BTN_STOP_PIN 14
#define LED_INDICATOR 2
#define DF_RX 16
#define DF_TX 17

// 3. OBJETOS
SoftwareSerial softSerial(DF_RX, DF_TX);
DFRobotDFPlayerMini myDFPlayer;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// 4. VARIABLES DE ESTADO
bool isAlarmActive = false;
int activeTrack = 0;
String activeTaskId = "";
unsigned long lastCheck = 0;
const long checkInterval = 30000; // Revisar cada 30 segundos
String lastTriggeredMinute = "";

// Configuración NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800; // Ajustar según tu país (ej: -3h = -10800s)
const int   daylightOffset_sec = 0;

void syncTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Sincronizando hora...");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) Serial.println(&timeinfo, "Hora actual: %H:%M:%S");
}

void setup() {
  Serial.begin(115200);
  softSerial.begin(9600);
  
  pinMode(BTN_STOP_PIN, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);
  
  if (!myDFPlayer.begin(softSerial)) {
    Serial.println("Error: DFPlayer no detectado.");
  } else {
    Serial.println("DFPlayer Mini Online.");
    myDFPlayer.volume(20); // Volumen 0-30
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Conectado.");
  
  syncTime();

  config.api_key = API_KEY;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void markTaskAsDone(String taskId) {
  // Registra en Firestore que el paciente tocó el botón
  String path = "users/" + String(USER_UID) + "/routine/" + taskId;
  FirebaseJson json;
  // Firestore requiere un formato específico para timestamps o podemos usar strings
  json.set("fields/lastCompletedAt/stringValue", "Completado a las " + String(millis())); 
  
  if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), json.raw(), "lastCompletedAt")) {
    Serial.println("Evento registrado en Firebase.");
  }
}

void checkRoutine() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;

  char currentTime[6];
  sprintf(currentTime, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  String nowStr = String(currentTime);

  if (nowStr == lastTriggeredMinute) return;

  Serial.print("Buscando tareas para las: "); Serial.println(nowStr);

  // Obtener documentos de la colección routine
  String path = "users/" + String(USER_UID) + "/routine";
  
  if (Firebase.Firestore.listDocuments(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str())) {
    FirebaseJson data;
    data.setJsonData(fbdo.payload().c_str());
    
    FirebaseJsonArray docs;
    data.get(docs, "documents");
    
    for (size_t i = 0; i < docs.size(); i++) {
      FirebaseJsonData result;
      docs.get(result, i);
      FirebaseJson doc;
      doc.setJsonData(result.stringValue.c_str());
      
      FirebaseJsonData taskTime, taskTrack, taskId, taskActive;
      doc.get(taskTime, "fields/time/stringValue");
      doc.get(taskTrack, "fields/trackNumber/integerValue");
      doc.get(taskActive, "fields/isActive/booleanValue");
      doc.get(taskId, "name"); // El path completo del doc

      if (taskActive.boolValue && taskTime.stringValue == nowStr) {
        isAlarmActive = true;
        activeTrack = taskTrack.intValue;
        
        // Extraer solo el ID del final del path
        String fullPath = taskId.stringValue;
        activeTaskId = fullPath.substring(fullPath.lastIndexOf('/') + 1);
        
        lastTriggeredMinute = nowStr;
        myDFPlayer.play(activeTrack);
        Serial.printf("¡ALARMA! Pista: %d\n", activeTrack);
        break;
      }
    }
  }
}

void loop() {
  if (isAlarmActive) {
    // Parpadeo del LED
    digitalWrite(LED_INDICATOR, (millis() / 500) % 2);
    
    // Si el audio terminó y la alarma sigue activa, repetir
    // Nota: Dependiendo de la versión del DFPlayer, esto puede variar
    static unsigned long lastPlay = 0;
    if (millis() - lastPlay > 10000) { // Repetir cada 10s si es necesario
       myDFPlayer.play(activeTrack);
       lastPlay = millis();
    }

    if (digitalRead(BTN_STOP_PIN) == LOW) {
      Serial.println("Paciente presionó el botón.");
      myDFPlayer.stop();
      digitalWrite(LED_INDICATOR, LOW);
      isAlarmActive = false;
      markTaskAsDone(activeTaskId);
      delay(1000);
    }
  } else {
    if (millis() - lastCheck > checkInterval) {
      lastCheck = millis();
      checkRoutine();
    }
  }
}
