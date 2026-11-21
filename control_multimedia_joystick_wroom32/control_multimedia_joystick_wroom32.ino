// Control multimedia BLE con ESP32-WROOM-32 (DevKit V1, 30 pines) + joystick
// Arriba: subir volumen | Abajo: bajar volumen
// Derecha: siguiente canción | Izquierda: canción anterior
// Pulsar joystick (SW): Play / Pausa
//
// IMPORTANTE: la librería clásica "ESP32 BLE Keyboard" de T-vK NO
// compila con el core ESP32 3.3.12 que tienes instalado (el error
// std::string vs String es de esa librería, no de tu código).
// Por eso aquí se usa HijelHID_BLEKeyboard, que sí está hecha
// para el core 3.x y usa NimBLE.
//
// Librerías (Arduino IDE > Herramientas > Gestionar librerías,
// instalar en este orden):
//   1. NimBLE-Arduino, de h2zero, versión 2.3.8 o mayor
//   2. HijelHID_BLEKeyboard, de hijelhub
// Placa: DOIT ESP32 DEVKIT V1  o  ESP32 Dev Module
//
// Conexiones (joystick a 3.3V, NO a 5V, el ADC es de 3.3V):
//   Joystick VCC  -> 3V3
//   Joystick GND  -> GND
//   Joystick VRx  -> D32  (GPIO 32, ADC1)
//   Joystick VRy  -> D33  (GPIO 33, ADC1)
//   Joystick SW   -> D4   (GPIO 4, a GND al pulsar, pull-up interno)
// GPIO 32 y 33 son ADC1, así que siguen funcionando aunque el
// Bluetooth/WiFi esté activo. Si arriba/abajo o izquierda/derecha
// te salen al revés, intercambia las dos líneas de ese eje.

#include <HijelHID_BLEKeyboard.h>

HijelHID_BLEKeyboard keyboard("ESP32-WROOM-Media", "Crucito", 100);

const uint8_t PIN_VRX = 32;  // eje X: izquierda / derecha
const uint8_t PIN_VRY = 33;  // eje Y: arriba / abajo
const uint8_t PIN_SW  = 4;   // pulsar joystick

// El centro del joystick ronda 2048 (ADC 0-4095).
// Zona muerta para que no se dispare solo.
const int UMBRAL_BAJO = 1000;
const int UMBRAL_ALTO = 3100;

// Volumen: repetir mientras se mantiene empujado, cada 180 ms
const unsigned long REPETIR_VOLUMEN_MS = 180;
unsigned long ultimoVolumen = 0;

// Siguiente / anterior: solo un envío por empujón
bool enZonaX = false;
bool swPresionadoAntes = false;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SW, INPUT_PULLUP);
  analogReadResolution(12); // 0-4095
  keyboard.begin();
  Serial.println("Control multimedia BLE listo. Busca 'ESP32-WROOM-Media' en Bluetooth.");
}

void loop() {
  if (!keyboard.isConnected()) {
    delay(200);
    return;
  }

  int x = analogRead(PIN_VRX);
  int y = analogRead(PIN_VRY);
  bool sw = (digitalRead(PIN_SW) == LOW); // LOW = pulsado
  unsigned long ahora = millis();

  // --- Volumen: eje Y ---
  if (y < UMBRAL_BAJO) {
    if (ahora - ultimoVolumen >= REPETIR_VOLUMEN_MS) {
      keyboard.tap(MEDIA_VOLUME_UP);
      ultimoVolumen = ahora;
    }
  } else if (y > UMBRAL_ALTO) {
    if (ahora - ultimoVolumen >= REPETIR_VOLUMEN_MS) {
      keyboard.tap(MEDIA_VOLUME_DOWN);
      ultimoVolumen = ahora;
    }
  }

  // --- Siguiente / anterior: eje X, solo al entrar en la zona ---
  if (x > UMBRAL_ALTO && !enZonaX) {
    keyboard.tap(MEDIA_NEXT_TRACK);
    enZonaX = true;
  } else if (x < UMBRAL_BAJO && !enZonaX) {
    keyboard.tap(MEDIA_PREV_TRACK);
    enZonaX = true;
  } else if (x >= UMBRAL_BAJO && x <= UMBRAL_ALTO) {
    enZonaX = false; // volvió al centro, listo para el siguiente empujón
  }

  // --- Play / Pausa: pulsar el joystick, solo al presionar ---
  if (sw && !swPresionadoAntes) {
    keyboard.tap(MEDIA_PLAY_PAUSE);
    delay(50); // antirrebote mínimo
  }
  swPresionadoAntes = sw;

  delay(20);
}
