
/**
 * ==================================================================================
 * SafeDay v1.4 - Sistema de apoyo Alzheimer (ESP32 + DFPlayer Mini)
 * Hardware: ESP32, DFPlayer Mini, Pulsador (Pin 14), LED (Pin 2), Parlante.
 * 
 * VINCULACIÓN: Solo necesitas copiar tu USER_UID desde el Dashboard de la App.
 * No requiere email ni password en el código.
 * ==================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// 1. CONFIGURACIÓN DE RED
#define WIFI_SSID "TU_WIFI_NAME"
#define WIFI_PASSWORD "TU_WIFI_PASSWORD"

// 2. CONFIGURACIÓN DE FIREBASE (Copia esto desde tu Dashboard)
#define API_KEY "AIzaSyAdUgSVPEpdoLF15UleY65qm3PtjC82h-8"
#define FIREBASE_PROJECT_ID "safeday-29789405-ff4df"
#define USER_UID "COPIA_TU_UID_AQUI" 

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
int activeTrack = 0;
String activeTaskId = "";
unsigned long lastCheck = 0;
const long checkInterval = 20000; // Revisar cada 20 segundos
String lastTriggeredMinute = "";

// Configuración NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800; // Ajustar según tu país (ej: -3h = -10800s)
const int   daylightOffset_sec = 0;

void syncTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Sincronizando hora...");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    Serial.printf("Hora actual: %s\n", timeStr);
  }
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
    myDFPlayer.volume(20); 
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Conectado.");
  
  syncTime();

  // Configuración de Firebase (Sin Auth para lectura pública permitida por UID)
  config.api_key = API_KEY;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Sistema SafeDay Inicializado.");
}

void markTaskAsDone(String taskId) {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char doneTime[20];
  strftime(doneTime, sizeof(doneTime), "%H:%M", &timeinfo);
  
  String path = "users/" + String(USER_UID) + "/routine/" + taskId;
  FirebaseJson json;
  json.set("fields/lastCompletedAt/stringValue", String(doneTime)); 
  
  // Nota: Esto requiere que las reglas de Firestore permitan escritura por UID o pública.
  // En SafeDay permitimos escritura si el dispositivo conoce el UID.
  if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), json.raw(), "lastCompletedAt")) {
    Serial.println("Cumplimiento registrado en la nube.");
  } else {
    Serial.println("Error al registrar: " + fbdo.errorReason());
  }
}

void checkRoutine() {
  if (!Firebase.ready()) return;

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;

  char currentTime[6];
  sprintf(currentTime, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  String nowStr = String(currentTime);

  if (nowStr == lastTriggeredMinute) return;

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
      doc.get(taskId, "name");

      if (taskActive.boolValue && taskTime.stringValue == nowStr) {
        isAlarmActive = true;
        activeTrack = taskTrack.intValue;
        
        String fullPath = taskId.stringValue;
        activeTaskId = fullPath.substring(fullPath.lastIndexOf('/') + 1);
        
        lastTriggeredMinute = nowStr;
        myDFPlayer.play(activeTrack);
        Serial.printf("¡ALARMA! Tarea: %s, Audio: %d\n", activeTaskId.c_str(), activeTrack);
        break;
      }
    }
  }
}

void loop() {
  if (isAlarmActive) {
    // Parpadeo del LED mientras suena
    digitalWrite(LED_INDICATOR, (millis() / 500) % 2);
    
    // Repetir audio cada 15 seg si no se ha detenido
    static unsigned long lastPlay = 0;
    if (millis() - lastPlay > 15000) { 
       myDFPlayer.play(activeTrack);
       lastPlay = millis();
    }

    if (digitalRead(BTN_STOP_PIN) == LOW) {
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
