/**
 * ==================================================================================
 * Código de Prueba Simple para Sensor de Movimiento PIR (HC-SR501)
 *
 * Este sketch es para diagnosticar si tu sensor PIR funciona correctamente.
 * No utiliza WiFi ni Firebase.
 *
 * Cómo usarlo:
 * 1. Conecta el sensor PIR a tu ESP32:
 *    - VCC del sensor -> Pin 5V o 3.3V del ESP32
 *    - GND del sensor -> Pin GND del ESP32
 *    - OUT del sensor -> Pin 23 del ESP32 (puedes cambiarlo en la constante PIR_PIN)
 *
 * 2. Sube este código a tu ESP32.
 *
 * 3. Abre el Monitor Serie en tu IDE (Arduino IDE o PlatformIO) a una velocidad
 *    de 115200 baudios.
 *
 * 4. Al iniciar, el LED integrado del ESP32 parpadeará durante 10 segundos.
 *    Durante este tiempo, el sensor se está calibrando. ¡No te muevas frente a él!
 *
 * 5. Después de la calibración, el programa imprimirá "Sensor listo." en el monitor.
 *
 * 6. Ahora, mueve tu mano frente al sensor. Deberías ver:
 *    - El mensaje "¡Movimiento detectado!" en el Monitor Serie.
 *    - El LED integrado del ESP32 se encenderá.
 *
 * Si esto funciona, tu sensor está perfecto. Si no, revisa el cableado o
 * prueba con otro sensor.
 * ==================================================================================
 */

#include <Arduino.h>

// Define el pin donde está conectado el pin de SALIDA (OUT) del sensor PIR.
#define PIR_PIN 23

// Define el pin del LED integrado del ESP32 (generalmente es el 2).
#define LED_PIN 2

// Variable para guardar el estado del sensor.
int pirState = LOW;

void setup() {
  // Inicia la comunicación serie.
  Serial.begin(115200);

  // Configura los pines.
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // Apaga el LED al inicio.
  digitalWrite(LED_PIN, LOW);

  // --- Período de Calibración ---
  Serial.println("Calibrando el sensor PIR... (10 segundos)");
  Serial.println("Por favor, no te muevas frente al sensor.");

  // Haz parpadear el LED para indicar que está calibrando.
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }

  Serial.println("¡Sensor listo!");
}

void loop() {
  // Lee el valor actual del sensor PIR.
  int val = digitalRead(PIR_PIN);

  // Compara el valor con el estado anterior.
  if (val == HIGH) { // Si el sensor detecta movimiento...
    digitalWrite(LED_PIN, HIGH); // Encender el LED.

    if (pirState == LOW) {
      // Si el estado anterior era BAJO, significa que el movimiento acaba de empezar.
      Serial.println("¡Movimiento detectado!");
      pirState = HIGH; // Actualiza el estado a ALTO.
    }
  } else { // Si el sensor NO detecta movimiento...
    digitalWrite(LED_PIN, LOW); // Apagar el LED.

    if (pirState == HIGH) {
      // Si el estado anterior era ALTO, significa que el movimiento acaba de terminar.
      Serial.println("Movimiento terminado.");
      pirState = LOW; // Actualiza el estado a BAJO.
    }
  }

  // Pequeña pausa para no saturar el monitor serie.
  delay(100);
}
