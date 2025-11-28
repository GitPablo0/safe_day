/**
 * ==================================================================================
 * Código de prueba OFFLINE para un sistema de alarma PIR.
 * VERSIÓN SIN WIFI NI FIREBASE.
 *
 * --- INSTRUCCIONES ---
 * 1. Sube este código a tu ESP32.
 * 2. Abre el Monitor Serie a 115200 baudios.
 * 3. El sistema se "armará" automáticamente.
 * 4. Mueve la mano frente al sensor PIR.
 * 5. Deberías ver el mensaje "¡PRESENCIA DETECTADA!" en el monitor,
 *    el LED rojo encenderse y el buzzer sonar de forma intermitente.
 * ==================================================================================
 */

#include <Arduino.h>

// ===============================================
// 1. PINES Y SENSORES
// ===============================================
#define PIR_PIN 23       // Pin para el sensor de movimiento PIR
#define LED_ROJO_PIN 16  // Pin para el LED rojo (detección de presencia)
#define LED_AZUL_PIN 15  // Pin para el LED azul (sistema armado)
#define BUZZER_PIN 22    // Pin para el buzzer

// ===============================================
// 2. VARIABLES DE ESTADO (MODO OFFLINE)
// ===============================================
bool isArmed = true; // El sistema se arma por defecto para la prueba
bool presenceDetected = false;
unsigned long lastBuzzerToggleTime = 0;
const long buzzerToggleInterval = 250; // Intervalo para el sonido del buzzer

void setup() {
  Serial.begin(115200);
  Serial.println("===============================");
  Serial.println("Inicializando sistema en MODO OFFLINE...");
  Serial.println("===============================");


  // Inicializar pines
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  pinMode(LED_AZUL_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Apagar todos los actuadores al inicio
  digitalWrite(LED_ROJO_PIN, LOW);
  digitalWrite(LED_AZUL_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  if (isArmed) {
    Serial.println("Sistema ARMADO. Esperando detección de movimiento...");
  } else {
    Serial.println("Sistema DESARMADO.");
  }
}

void loop() {
  // --- 1. LECTURA DEL SENSOR PIR ---
  bool currentPresence = digitalRead(PIR_PIN) == HIGH;

  // Si el sistema está armado y se detecta presencia por primera vez
  if (isArmed && currentPresence && !presenceDetected) {
      Serial.println("¡PRESENCIA DETECTADA!");
      presenceDetected = true;
      // Para probar de nuevo, tendrías que reiniciar el ESP32
      // o añadir un botón para resetear 'presenceDetected'.
  }

  // --- 2. CONTROL DE ACTUADORES ---
  digitalWrite(LED_AZUL_PIN, isArmed); // LED azul encendido si el sistema está armado

  if (presenceDetected) {
    // Si hay presencia, el LED rojo se enciende fijo
    digitalWrite(LED_ROJO_PIN, HIGH);
    // Y el buzzer emite un sonido intermitente
    unsigned long currentMillis = millis();
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
