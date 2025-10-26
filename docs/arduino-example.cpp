/**
 * ==================================================================================
 * Este es un archivo de ejemplo para conectar un microcontrolador (como un ESP32)
 * a Firebase Firestore para el proyecto NeuroSense.
 *
 * Dependencias de la biblioteca de Arduino (Gestor de Bibliotecas):
 * 1. Firebase Arduino Client Library for ESP32 and ESP8266 by Mobizt
 * 2. ArduinoJson by Benoit Blanchon
 * ==================================================================================
 */
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>

// ===============================================================
// 1. CONFIGURACIÓN DE WIFI
// ===============================================================
// Reemplaza con tus credenciales de Wi-Fi
#define WIFI_SSID "TU_WIFI_SSID"
#define WIFI_PASSWORD "TU_WIFI_CONTRASENA"

// ===============================================================
// 2. CONFIGURACIÓN DE FIREBASE
// ===============================================================
// Ve a tu consola de Firebase -> Configuración del proyecto -> Cuentas de servicio
// -> Crear nueva clave privada para obtener el JSON del SDK de administrador.
// O, para un enfoque más simple (menos seguro), puedes usar una clave de API web.
//
// Reemplaza con la clave de API web de tu proyecto de Firebase y la URL de la base de datos.
// Se encuentran en Consola de Firebase -> Configuración del proyecto -> General
#define API_KEY "AIzaSyCMmir0FR57YVoVGQ9QCUG4oG7bUEF3w7I" // Esta es la clave de API web
#define DATABASE_URL "studio-4253330981-dcdc9.firebaseio.com" // Tu URL de Realtime Database (usada para la autenticación)
#define FIREBASE_PROJECT_ID "studio-4253330981-dcdc9"

// Objeto de Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===============================================================
// 3. PINES Y SENSORES (EJEMPLO)
// ===============================================================
// Reemplaza con los pines reales que estés usando.
const int TEMP_SENSOR_PIN = 4;
const int SOUND_SENSOR_PIN = 34;
const int HUMIDITY_SENSOR_PIN = 5; // Suponiendo un sensor DHT

// ===============================================================
// 4. LÓGICA DE LA APLICACIÓN
// ===============================================================
unsigned long sendDataPrevMillis = 0;
// Envía datos a Firebase cada 2 segundos
const long sendDataInterval = 2000;

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando...");

  // Conectar a Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Conectado con IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Configurar Firebase
  config.api_key = API_KEY;
  auth.user.email = "device@neurosense.com"; // Usando autenticación anónima/personalizada
  auth.user.password = "device_password"; // No es crítico para anónimos, pero requerido
  config.database_url = DATABASE_URL;

  // Asignar las funciones de devolución de llamada para la gestión del token
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > sendDataInterval)) {
    sendDataPrevMillis = millis();

    // 1. Leer datos de los sensores (implementa estas funciones según tu hardware)
    float temperature = 25.5; //readTemperature();
    float humidity = 60.1; //readHumidity();
    int soundLevel = 250; //readSoundLevel();

    Serial.println("------------------------------------");
    Serial.print("Temperatura: "); Serial.println(temperature);
    Serial.print("Humedad: "); Serial.println(humidity);
    Serial.print("Nivel de sonido: "); Serial.println(soundLevel);
    Serial.println("------------------------------------");

    // 2. Crear el objeto JSON de contenido de Firestore
    // El formato debe coincidir con la estructura que espera la aplicación web.
    // documents: [{ fields: { key: { valueType: value } } }]
    String jsonData = "";
    jsonData += "{'fields': {";
    jsonData += "'temperature': {'doubleValue': " + String(temperature) + "},";
    jsonData += "'humidity': {'doubleValue': " + String(humidity) + "},";
    jsonData += "'soundLevel': {'integerValue': " + String(soundLevel) + "}";
    jsonData += "}}";

    // 3. Enviar datos a Firestore
    // La ruta es 'sensorData/latest'
    // Usamos patch para actualizar el documento, que es más eficiente que sobreescribirlo.
    String documentPath = "sensorData/latest";
    Serial.println("Enviando datos a Firestore en la ruta: " + documentPath);
    
    // Si la actualización falla, intenta crear el documento por si no existe
    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), jsonData.c_str(), "temperature,humidity,soundLevel")) {
        Serial.printf("Envío de datos exitoso: %s\n", fbdo.payload().c_str());
    } else {
        Serial.printf("Error al enviar datos: %s\n", fbdo.errorReason().c_str());
        Serial.println("Intentando crear el documento...");
        // Si 'patchDocument' falla porque el documento no existe, 'createDocument' lo creará.
        if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", "sensorData", jsonData.c_str())) {
            Serial.printf("Documento creado exitosamente: %s\n", fbdo.payload().c_str());
        } else {
            Serial.printf("Error al crear el documento: %s\n", fbdo.errorReason().c_str());
        }
    }
  }
}
