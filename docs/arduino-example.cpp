/**
 * ==================================================================================
 * Este es un archivo de ejemplo para conectar un microcontrolador (como un ESP32)
 * a Firebase Firestore para el proyecto NeuroSense.
 *
 * Dependencias de la biblioteca de Arduino (Gestor de Bibliotecas):
 * 1. Firebase Arduino Client Library for ESP32 and ESP8266 by Mobizt
 * 2. ArduinoJson by Benoit Blanchon
 * 3. DHT sensor library by Adafruit
 * 
 * --- SOLUCIÓN DE PROBLEMAS ---
 * Si encuentras errores de "SSL/TLS Handshake Failed" o "SSL internal timeout",
 * la causa más común es que el microcontrolador no tiene la hora correcta.
 * La conexión segura (SSL) requiere una hora precisa para validar los certificados.
 * ==================================================================================
 */
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include "DHT.h"

// ===============================================================
// 1. CONFIGURACIÓN DE WIFI
// ===============================================================
// Reemplaza con tus credenciales de Wi-Fi
#define WIFI_SSID "Telecentro-416b"
#define WIFI_PASSWORD "ATNMFMY2MM5Q"

// ===============================================================
// 2. CONFIGURACIÓN DE FIREBASE
// ===============================================================
// Reemplaza con la clave de API web de tu proyecto de Firebase.
#define API_KEY "AIzaSyCMmir0FR57YVoVGQ9QCUG4oG7bUEF3w7I"
#define FIREBASE_PROJECT_ID "studio-4253330981-dcdc9"

// Objetos de Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===============================================================
// 3. PINES Y SENSORES (DHT22 y KY-037)
// ===============================================================
#define DHTPIN 4         // Pin de datos para el DHT22
#define DHTTYPE DHT22    // Definiendo el tipo de sensor como DHT22
const int SOUND_SENSOR_PIN = 34; // Pin analógico para el KY-037

DHT dht(DHTPIN, DHTTYPE); // Inicializa el objeto sensor DHT

// Pines para los LEDs de estado
const int LED_VERDE_PIN = 12;
const int LED_NARANJA_PIN = 13;
const int LED_ROJO_PIN = 14;

// Umbrales por defecto (se intentarán leer desde Firebase)
float soundThresholdLow = 300;
float soundThresholdMedium = 380;
float tempThresholdVerde = 24;
float tempThresholdNaranja = 28;

// ===============================================================
// 4. LÓGICA DE LA APLICACIÓN
// ===============================================================
unsigned long sendDataPrevMillis = 0;
const long sendDataInterval = 15000; // 15 segundos

float readTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {
    Serial.println("Error al leer la temperatura del sensor DHT!");
    return 0; // Devuelve 0 o un valor de error si falla
  }
  return t;
}

float readHumidity() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Error al leer la humedad del sensor DHT!");
    return 0; // Devuelve 0 o un valor de error si falla
  }
  return h;
}

int readSoundLevel() {
  return analogRead(SOUND_SENSOR_PIN);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando...");
  
  dht.begin(); // Inicializa el sensor DHT
  delay(2000); // Pequeño retardo para que el sensor DHT se estabilice

  // Configurar pines de LEDs como salida
  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_NARANJA_PIN, OUTPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  digitalWrite(LED_VERDE_PIN, LOW);
  digitalWrite(LED_NARANJA_PIN, LOW);
  digitalWrite(LED_ROJO_PIN, LOW);

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

  // Pausa para estabilizar la conexión
  delay(1000); 

  Serial.println("Inicializando Firebase...");

  // Asignar la API Key para la configuración
  config.api_key = API_KEY;
  
  // Como no estamos usando autenticación de usuario, el objeto auth puede estar vacío
  // pero aún se debe pasar a la función begin.
  Firebase.begin(&config, &auth);
  
  Firebase.reconnectWiFi(true);
}

void loop() {
  // Se comprueba si millis() ha sobrepasado el intervalo, no si Firebase está "listo"
  if (millis() - sendDataPrevMillis > sendDataInterval) {
    sendDataPrevMillis = millis();

    // 1. Leer datos de los sensores reales
    float temperature = readTemperature();
    float humidity = readHumidity();
    int soundLevel = readSoundLevel();

    Serial.println("------------------------------------");
    Serial.println("Intentando enviar datos a Firebase...");
    Serial.print("Temperatura: "); Serial.println(temperature);
    Serial.print("Humedad: "); Serial.println(humidity);
    Serial.print("Nivel de sonido: "); Serial.println(soundLevel);
    Serial.println("------------------------------------");

    // Lógica para controlar los LEDs
    int estadoSonido = 1; // 1: Verde, 2: Naranja, 3: Rojo
    if (soundLevel >= soundThresholdLow) estadoSonido = 2;
    if (soundLevel >= soundThresholdMedium) estadoSonido = 3;

    int estadoTemp = 1;
    if (temperature >= tempThresholdVerde) estadoTemp = 2;
    if (temperature >= tempThresholdNaranja) estadoTemp = 3;
    
    int ledStatus = max(estadoSonido, estadoTemp);

    digitalWrite(LED_VERDE_PIN, ledStatus == 1);
    digitalWrite(LED_NARANJA_PIN, ledStatus == 2);
    digitalWrite(LED_ROJO_PIN, ledStatus == 3);

    Serial.printf("Estado de LEDs determinado: %d\n", ledStatus);

    // 2. Crear el objeto JSON de contenido de Firestore
    // El formato Firestore REST API es requerido por la biblioteca.
    String jsonData = "";
    jsonData += "{'fields': {";
    jsonData += "'temperature': {'doubleValue': " + String(temperature) + "},";
    jsonData += "'humidity': {'doubleValue': " + String(humidity) + "},";
    jsonData += "'soundLevel': {'integerValue': " + String(soundLevel) + "}";
    jsonData += "}}";

    // 3. Enviar datos a Firestore
    String documentPath = "sensorData/latest";
    Serial.println("Enviando datos a Firestore en la ruta: " + documentPath);
    
    // Usamos patch para actualizar el documento. Esto es más eficiente y creará el documento si no existe.
    // El UID del usuario no es necesario aquí.
    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), jsonData.c_str(), "")) {
        Serial.printf("Envío de datos exitoso: %s\n", fbdo.payload().c_str());
    } else {
        Serial.printf("Error al enviar datos: %s\n", fbdo.errorReason().c_str());
    }
  }
}
