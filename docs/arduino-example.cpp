
/**
 * ==================================================================================
 * SafeDay v1.8 - Sistema de apoyo Alzheimer (ESP32 + DFPlayer Mini)
 * 
 * INSTRUCCIONES:
 * 1. Crea un usuario en Firebase Auth: "esp32@safeday.com" / "safeday123".
 * 2. El UID ya está configurado abajo.
 * 3. Conexiones: 
 *    - DFPlayer RX -> ESP32 TX2 (GPIO 17)
 *    - DFPlayer TX -> ESP32 RX2 (GPIO 16)
 *    - Botón Stop -> GPIO 4 (conectar a GND al presionar)
 *    - LED -> GPIO 2 (conectar a GND con resistencia)
 * ==================================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include "DFRobotDFPlayerMini.h"

// ===============================================================
// 1. CONFIGURACIÓN DE USUARIO Y WIFI
// ===============================================================
#define USER_UID "WWxcJu01odP4dFIp42sHjKUliQ13"

struct WiFiNetwork {
  const char* ssid;
  const char* password;
};

WiFiNetwork knownNetworks[] = {
  {"Telecentro-416b", "ATNMFMY2MM5Q"},
  {"Xiomi", "NTXR1956"},
  {"Pablooo13", "brenda123"},
  {"Estudiantes", "Escuelas_2025"}
};
const int numKnownNetworks = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

// ===============================================================
// 2. CONFIGURACIÓN DE FIREBASE
// ===============================================================
#define API_KEY "AIzaSyAdUgSVPEpdoLF15UleY65qm3PtjC82h-8"
#define FIREBASE_PROJECT_ID "safeday-29789405-ff4df"

// ===============================================
// 3. PINES
// ===============================================
#define BTN_STOP_PIN 4    // Botón físico en GPIO 4
#define LED_INDICATOR 2   // LED indicador en GPIO 2
#define DF_RX 16          // TX del DFPlayer -> RX2 del ESP32
#define DF_TX 17          // RX del DFPlayer -> TX2 del ESP32

// ===============================================
// 4. OBJETOS Y VARIABLES
// ===============================================
DFRobotDFPlayerMini myDFPlayer;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool isAlarmActive = false;
int activeTrack = 0;
String activeTaskId = "";
unsigned long lastFirestoreRead = 0;
const long firestoreReadInterval = 30000; // Revisar rutina cada 30 seg
String lastTriggeredMinute = "";

// Declaraciones
void connectToWiFi();
bool syncTime();
void tokenStatusCallback(TokenInfo info);
void checkRoutineInFirestore();
void markTaskAsDone(String taskId);

void setup() {
  Serial.begin(115200);
  
  // Inicializar Serial2 para el DFPlayer (Hardware Serial nativo de ESP32)
  Serial2.begin(9600, SERIAL_8N1, DF_RX, DF_TX);
  
  pinMode(BTN_STOP_PIN, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);
  
  Serial.println("Iniciando SafeDay...");
  if (!myDFPlayer.begin(Serial2)) {
    Serial.println("Error: DFPlayer no detectado.");
  } else {
    Serial.println("DFPlayer Mini Online.");
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

  Serial.println("SafeDay Inicializado.");
}

void loop() {
  unsigned long currentMillis = millis();

  if (!Firebase.ready()) return;

  if (isAlarmActive) {
    // Parpadeo de LED mientras la alarma suena
    digitalWrite(LED_INDICATOR, (millis() / 500) % 2); 
    
    static unsigned long lastPlay = 0;
    if (millis() - lastPlay > 12000) { 
       myDFPlayer.play(activeTrack);
       lastPlay = millis();
    }

    // El botón detiene la alarma
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
  
  // IMPORTANTE: Se pasan 9 argumentos exactos. pageSize (5to arg) DEBE ser un entero 0.
  if (Firebase.Firestore.listDocuments(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), 0, "", "", "", false)) {
    FirebaseJson data;
    data.setJsonData(fbdo.payload().c_str());
    
    FirebaseJsonData result;
    data.get(result, "documents");
    
    // Corregido: Comparar typeNum (entero) con el enum de la librería
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

        // Usar .to<T>() para conversiones seguras
        if (taskActive.to<bool>() && taskTime.to<String>() == nowStr) {
          isAlarmActive = true;
          activeTrack = taskTrack.to<int>();
          String fullPath = taskId.to<String>();
          activeTaskId = fullPath.substring(fullPath.lastIndexOf('/') + 1);
          lastTriggeredMinute = nowStr;
          myDFPlayer.play(activeTrack);
          Serial.printf("¡ACTIVADA! Tarea: %s, Audio: %d\n", activeTaskId.c_str(), activeTrack);
          break;
        }
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
  
  // Patch para actualizar solo el campo de cumplimiento
  if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), content.c_str(), "lastCompletedAt")) {
    Serial.println("Cumplimiento registrado en Firestore.");
  }
}

void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_ready) {
    Serial.println("Autenticado con éxito.");
  }
}

bool syncTime() {
  configTime(-10800, 0, "pool.ntp.org"); // GMT-3 (Ajustar según tu zona)
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
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500); Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConectado a WiFi.");
        return;
    }
  }
}
