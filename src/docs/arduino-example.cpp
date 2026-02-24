
/**
 * ==================================================================================
 * SafeDay v2.3 - Sistema de apoyo Alzheimer (ESP32 + WiFi Dinámico)
 * 
 * MEJORAS:
 * 1. Detección de rutina restaurada: Se corrigió la consulta a Firestore.
 * 2. Audio infinito: El audio se repite hasta presionar el botón (.loop).
 * 3. Lógica de botón robusta: Diferencia clic corto (Stop) de clic largo (Reset WiFi).
 * 4. Depuración: Mensajes en Monitor Serial para ver el estado de la búsqueda.
 * ==================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include "DFRobotDFPlayerMini.h"

// ===============================================================
// 1. CONFIGURACIÓN FIJA
// ===============================================================
#define USER_UID "WWxcJu01odP4dFIp42sHjKUliQ13"
#define API_KEY "AIzaSyAdUgSVPEpdoLF15UleY65qm3PtjC82h-8"
#define FIREBASE_PROJECT_ID "safeday-29789405-ff4df"

#define BTN_STOP_PIN 4    
#define LED_INDICATOR 2   
#define DF_RX 16          
#define DF_TX 17          

// ===============================================
// 2. OBJETOS Y VARIABLES
// ===============================================
DFRobotDFPlayerMini myDFPlayer;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

bool isAlarmActive = false;
int activeTrack = 0;
String activeTaskId = "";
unsigned long lastFirestoreRead = 0;
const long firestoreReadInterval = 30000; 
String lastTriggeredMinute = "";

const char* AP_SSID = "SafeDay_Config";
bool isConfigMode = false;

// Declaraciones
void setupConfigPortal();
void handleRoot();
void handleSave();
bool syncTime();
void checkRoutineInFirestore();
void markTaskAsDone(String taskId);

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, DF_RX, DF_TX);
  
  pinMode(BTN_STOP_PIN, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);

  preferences.begin("safeday-wifi", false);
  
  if (!myDFPlayer.begin(Serial2)) {
    Serial.println("DFPlayer no detectado.");
  } else {
    myDFPlayer.volume(20); 
  }

  String savedSSID = preferences.getString("ssid", "");
  String savedPASS = preferences.getString("pass", "");

  if (savedSSID == "") {
    setupConfigPortal();
  } else {
    WiFi.begin(savedSSID.c_str(), savedPASS.c_str());
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
      delay(500); Serial.print(".");
      retries++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
      setupConfigPortal();
    } else {
      Serial.println("\nWiFi Conectado!");
    }
  }

  if (!isConfigMode) {
    syncTime();
    config.api_key = API_KEY;
    auth.user.email = "esp32@safeday.com";
    auth.user.password = "safeday123";
    
    Firebase.reconnectWiFi(true);
    Firebase.begin(&config, &auth);
    Serial.println("SafeDay Inicializado.");
  }
}

void loop() {
  if (isConfigMode) {
    dnsServer.processNextRequest();
    server.handleClient();
    digitalWrite(LED_INDICATOR, (millis() / 200) % 2); 
    return;
  }

  // --- LÓGICA DEL BOTÓN ---
  static unsigned long btnTimer = 0;
  bool btnPressed = (digitalRead(BTN_STOP_PIN) == LOW);

  if (btnPressed) {
    if (btnTimer == 0) btnTimer = millis();
    if (millis() - btnTimer > 5000) {
      preferences.putString("ssid", "");
      preferences.putString("pass", "");
      Serial.println("WiFi reseteado. Reiniciando...");
      delay(1000);
      ESP.restart();
    }
  } else {
    if (btnTimer > 0) {
      unsigned long duration = millis() - btnTimer;
      if (duration < 3000 && isAlarmActive) {
        Serial.println("Botón presionado: Deteniendo alarma.");
        myDFPlayer.stop();
        isAlarmActive = false;
        digitalWrite(LED_INDICATOR, LOW);
        markTaskAsDone(activeTaskId);
      }
      btnTimer = 0;
    }
  }

  if (!Firebase.ready()) return;

  if (isAlarmActive) {
    digitalWrite(LED_INDICATOR, (millis() / 500) % 2); 
  } else {
    digitalWrite(LED_INDICATOR, LOW);
    if (millis() - lastFirestoreRead > firestoreReadInterval) {
      lastFirestoreRead = millis();
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
  
  Serial.println("Revisando rutina para la hora: " + nowStr);

  String path = "users/" + String(USER_UID) + "/routine";
  
  // Usamos "" para databaseId para mayor compatibilidad
  if (Firebase.Firestore.listDocuments(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), 0, "", "", "", false)) {
    FirebaseJson json;
    json.setJsonData(fbdo.payload().c_str());
    FirebaseJsonData result;
    json.get(result, "documents");
    
    if (result.success && result.typeNum == FirebaseJson::JSON_ARRAY) {
      FirebaseJsonArray docs;
      result.getArray(docs);
      for (size_t i = 0; i < docs.size(); i++) {
        FirebaseJsonData docResult;
        docs.get(docResult, i);
        FirebaseJson doc;
        doc.setJsonData(docResult.stringValue.c_str());
        
        FirebaseJsonData taskTime, taskTrack, taskId, taskActive;
        doc.get(taskTime, "fields/time/stringValue");
        doc.get(taskTrack, "fields/trackNumber/integerValue");
        doc.get(taskActive, "fields/isActive/booleanValue");
        doc.get(taskId, "name");

        if (taskActive.to<bool>() && taskTime.to<String>() == nowStr) {
          isAlarmActive = true;
          activeTrack = taskTrack.to<int>();
          String fullPath = taskId.to<String>();
          activeTaskId = fullPath.substring(fullPath.lastIndexOf('/') + 1);
          lastTriggeredMinute = nowStr;
          
          myDFPlayer.loop(activeTrack); // Sonido infinito
          Serial.println("ALARMA ACTIVADA: " + activeTaskId);
          break;
        }
      }
    } else {
      Serial.println("No se encontraron actividades programadas.");
    }
  } else {
    Serial.println("Error de conexión con Firestore: " + fbdo.errorReason());
  }
}

void markTaskAsDone(String taskId) {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char doneTime[20];
  strftime(doneTime, sizeof(doneTime), "%H:%M", &timeinfo);
  
  String path = "users/" + String(USER_UID) + "/routine/" + taskId;
  String content = "{\"fields\":{\"lastCompletedAt\":{\"stringValue\":\"" + String(doneTime) + "\"}}}";
  
  if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), content.c_str(), "lastCompletedAt")) {
    Serial.println("Actividad marcada como completada en la nube.");
  }
}

void setupConfigPortal() {
  isConfigMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  Serial.println("Portal iniciado: SafeDay_Config");
}

void handleRoot() {
  String html = "<html><body style='text-align:center;'><h1>SafeDay Config</h1>";
  html += "<form action='/save' method='POST'>WiFi: <input name='ssid'><br>Pass: <input name='pass' type='password'><br><input type='submit'></form></body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  preferences.putString("ssid", server.arg("ssid"));
  preferences.putString("pass", server.arg("pass"));
  server.send(200, "text/html", "Guardado. Reiniciando...");
  delay(2000);
  ESP.restart();
}

bool syncTime() {
  configTime(-10800, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  int r = 0;
  while (now < 24 * 3600 && r++ < 20) { delay(500); now = time(nullptr); }
  return now > 24 * 3600;
}
