/*
  Lógica de control para un sistema de alarma con sensor PIR conectado a Firebase.

  Componentes:
  - ESP32
  - Sensor PIR (Movimiento)
  - LED Rojo (Indicador de Alarma/Presencia)
  - LED Azul (Indicador de Sistema Armado)
  - Buzzer (Sirena)

  Funcionalidad:
  - El dispositivo escucha en tiempo real el documento /alarm/state en Firestore.
  - El estado 'isArmed' del documento determina si el sistema está activo.
    - Si 'isArmed' es true, el LED Azul se enciende.
    - Si 'isArmed' es false, todos los indicadores se apagan.
  - Cuando el sistema está armado, el sensor PIR monitorea la presencia.
  - Si se detecta presencia:
    - El dispositivo actualiza 'presenceDetected' a true en Firestore.
    - El LED Rojo se enciende y el buzzer suena durante 'alarmDuration' segundos.
  - La app puede desarmar el sistema (poniendo 'isArmed' a false), lo que silencia
    la alarma y apaga los LEDs.
*/

// --- PINES ---
// Define aquí los pines que estás utilizando en tu ESP32
#define PIR_PIN 23       // Pin para el sensor de movimiento PIR
#define LED_ROJO_PIN 19  // Pin para el LED rojo (detección de presencia)
#define LED_AZUL_PIN 21  // Pin para el LED azul (sistema armado)
#define BUZZER_PIN 22    // Pin para el buzzer

// --- CONFIGURACIÓN DE WIFI Y FIREBASE ---
// Reemplaza con tus credenciales
const char* WIFI_SSID = "PON_TU_WIFI_SSID_AQUI";
const char* WIFI_PASSWORD = "PON_TU_WIFI_PASSWORD_AQUI";
const char* FIREBASE_UID = "PON_TU_UID_AQUI"; // UID de autenticación anónima

// --- VARIABLES GLOBALES DE ESTADO ---
bool isArmed = false;
bool presenceDetected = false;
int alarmDuration = 10; // Duración por defecto de la alarma en segundos

unsigned long alarmTriggeredTimestamp = 0; // Momento en que se disparó la alarma
bool isAlarmSounding = false;              // Indica si el buzzer está sonando

void setup() {
  Serial.begin(115200);

  // Inicializar pines
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  pinMode(LED_AZUL_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Apagar todos los actuadores al inicio
  digitalWrite(LED_ROJO_PIN, LOW);
  digitalWrite(LED_AZUL_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Conectar a WiFi y Firebase (pseudo-código, reemplaza con tu implementación real)
  // connectToWiFi(WIFI_SSID, WIFI_PASSWORD);
  // setupFirebase(FIREBASE_UID);
  
  // Suscribirse a cambios en el documento de estado de la alarma (pseudo-código)
  // Firestore.subscribe("/alarm/state", handleAlarmStateChange);

  Serial.println("Sistema de Alarma Inicializado.");
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Lógica del Sensor PIR (solo si el sistema está armado)
  if (isArmed) {
    bool currentPresence = digitalRead(PIR_PIN) == HIGH;
    
    if (currentPresence && !presenceDetected) {
      // Se acaba de detectar presencia
      Serial.println("¡Presencia Detectada!");
      presenceDetected = true;
      alarmTriggeredTimestamp = currentMillis;
      isAlarmSounding = true;
      
      // Actualizar estado en Firestore (pseudo-código)
      // Firestore.update("/alarm/state", "{'presenceDetected': true}");
    }
  }

  // 2. Lógica de la Alarma (Buzzer y LED Rojo)
  if (isAlarmSounding) {
    // Si el tiempo de alarma ha pasado, la apagamos
    if (currentMillis - alarmTriggeredTimestamp > (unsigned long)alarmDuration * 1000) {
      isAlarmSounding = false;
      presenceDetected = false; // Resetear la detección
      
      // Actualizar estado en Firestore (pseudo-código)
      // Firestore.update("/alarm/state", "{'presenceDetected': false}");
      Serial.println("Alarma finalizada.");
    }
  }

  // 3. Control de Actuadores (LEDs y Buzzer)
  // LED Azul: Siempre refleja el estado 'isArmed'
  digitalWrite(LED_AZUL_PIN, isArmed ? HIGH : LOW);
  
  // LED Rojo y Buzzer: Solo se activan si la alarma está sonando
  if (isArmed && isAlarmSounding) {
    digitalWrite(LED_ROJO_PIN, HIGH);
    // Hacer sonar el buzzer (puedes usar tonos para un efecto más de alarma)
    tone(BUZZER_PIN, 1000); // Emite un tono de 1kHz
  } else {
    digitalWrite(LED_ROJO_PIN, LOW);
    noTone(BUZZER_PIN); // Apaga el buzzer
  }
  
  // Pequeña pausa para no saturar el loop
  delay(100); 
}

// --- CALLBACK DE FIREBASE ---
// Esta función se llamaría cada vez que haya un cambio en /alarm/state
void handleAlarmStateChange(const char* jsonData) {
  // Parsea el JSON que llega de Firestore (pseudo-código)
  // JsonDocument doc;
  // deserializeJson(doc, jsonData);
  
  // isArmed = doc["isArmed"];
  // alarmDuration = doc["alarmDuration"];

  Serial.print("Estado de Firebase recibido: isArmed -> ");
  Serial.println(isArmed ? "TRUE" : "FALSE");

  // Si se recibe un 'isArmed' false, es una orden para apagar todo.
  if (!isArmed) {
    isAlarmSounding = false;
    presenceDetected = false;
    // Actualizar el estado en Firestore para confirmar el apagado
    // Firestore.update("/alarm/state", "{'presenceDetected': false}");
    Serial.println("Alarma desarmada desde la App.");
  }
}
