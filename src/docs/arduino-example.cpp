
/**
 * ==================================================================================
 * SafeDay v1.7 - Sistema de apoyo Alzheimer (ESP32 + DFPlayer Mini)
 * 
 * INSTRUCCIONES:
 * 1. Crea un usuario en Firebase Auth: "esp32@safeday.com" / "safeday123".
 * 2. El UID ya está configurado abajo.
 * ==================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include "DFRobotDFPlayerMini.h"

#define USER_UID "WWxcJu01odP4dFIp42sHjKUliQ13"

struct WiFiNetwork {
  const char* ssid;
  const char* password;
};

WiFiNetwork knownNetworks[] = {
  {"Telecentro-416b", "ATNMFMY2MM5Q"},
  {"Xiomi", "NTXR1956"}
};
const int numKnownNetworks = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

#define API_KEY "AIzaSyAdUgSVPEpdoLF15UleY65qm3PtjC82h-8"
#define FIREBASE_PROJECT_ID "safeday-29789405-ff4df"

#define BTN_STOP_PIN 14
#define LED_INDICATOR 2
#define DF_RX 16
#define DF_TX 17

DFRobotDFPlayerMini myDFPlayer;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool isAlarmActive = false;
int activeTrack = 0;
String activeTaskId = "";
unsigned long lastFirestoreRead = 0;
const long firestoreReadInterval = 30000;
String lastTriggeredMinute = "";

void connectToWiFi();
bool syncTime();
void tokenStatusCallback(TokenInfo info);
void checkRoutineInFirestore();
void markTaskAsDone(String taskId);

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, DF_RX, DF_TX);
  
  pinMode(BTN_STOP_PIN, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);
  
  if (!myDFPlayer.begin(Serial2)) {
    Serial.println("Error DFPlayer.");
  } else {
    myDFPlayer.volume(20); 
  }

  connectToWiFi();
  syncTime();

  config.api_key = API_KEY;
  auth.user.email = "esp32@safeday.com";
  auth.user.password = "safeday123";

  config.token_status_callback = tokenStatusCallback;
  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);
}

void loop() {
  unsigned long currentMillis = millis();
  if (!Firebase.ready()) return;

  if (isAlarmActive) {
    digitalWrite(LED_INDICATOR, (millis() / 500) % 2); 
    static unsigned long lastPlay = 0;
    if (millis() - lastPlay > 12000) { 
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
    digitalWrite(LED_INDICATOR, LOW);
    if (currentMillis - lastFirestoreRead > firestoreReadInterval) {
      lastFirestoreRead = currentMillis;
      checkRoutineInFirestore();
    }
  }
}

void checkRoutineInFirestore() {
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
        break;
      }
    }
  }
}

void markTaskAsDone(String taskId) {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char doneTime[20];
  strftime(doneTime, sizeof(doneTime), "%H:%M", &timeinfo);
  String path = "users/" + String(USER_UID) + "/routine/" + taskId;
  String content = "{\"fields\":{\"lastCompletedAt\":{\"stringValue\":\"" + String(doneTime) + "\"}}}";
  Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), content.c_str(), "lastCompletedAt");
}

void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_ready) Serial.println("Autenticado.");
}

bool syncTime() {
  configTime(-10800, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  int retries = 0;
  while (now < 24 * 3600 && retries < 20) {
    delay(500); now = time(nullptr); retries++;
  }
  return now > 24 * 3600;
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  for (int i = 0; i < numKnownNetworks; i++) {
    WiFi.begin(knownNetworks[i].ssid, knownNetworks[i].password);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) { delay(500); Serial.print("."); }
    if (WiFi.status() == WL_CONNECTED) return;
    WiFi.disconnect();
  }
}
