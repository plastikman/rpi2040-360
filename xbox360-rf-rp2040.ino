// Xbox 360 S (Slim / "Boron") FPM -> USB desktop adapter, RP2040-Zero edition.
//
// The RP2040 replaces the console SMC toward the Front Panel Module. It is
// NOT in the USB data path: the FPM's USB lines run straight to the PC and
// enumerate on power alone. This firmware only drives the control bus:
//   - power-on: send the mandatory Slim "start module" command, then the
//     ring-of-light boot animation (visible confirmation the bus works)
//   - sync button: trigger controller binding (11-bit Slim sync frame)
//
// Protocol per the working Slim implementation ginokgx/xbox360slimRF: the
// MODULE generates a slow (~hundreds of Hz) clock; we sync to it.
//
// Onboard RGB status (WS2812 on GP16):
//   solid green  -> running
//   blue flash   -> command sent OK
//   red flash    -> control-bus timeout (module didn't clock)
//
// Board: Waveshare RP2040-Zero   FQBN: rp2040:rp2040:waveshare_rp2040_zero
// Wiring/pinout: docs/wiring.md

#include <Adafruit_NeoPixel.h>
#include "XboxRF.h"

constexpr uint8_t PIN_BUTTON = 2;   // sync button to GND
constexpr uint8_t PIN_DATA   = 3;   // FPM pin 2  C_DATA
constexpr uint8_t PIN_CLK    = 4;   // FPM pin 3  C_CLK (module drives it)
constexpr uint8_t PIN_RGB    = 16;  // onboard WS2812

constexpr uint16_t DEBOUNCE_MS    = 30;
constexpr uint16_t BOOT_WAIT_MS   = 3500;  // let the FPM boot before talking
constexpr uint16_t CMD_GAP_MS     = 50;
constexpr uint16_t SYNC_WINDOW_MS = 4000;  // searching/pairing window
constexpr uint8_t  LVL            = 32;

XboxRF rf(PIN_DATA, PIN_CLK);
Adafruit_NeoPixel pixel(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

void led(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}
void ledIdle() { led(0, LVL, 0); }

void ledResult(bool ok) {
  for (uint8_t i = 0; i < (ok ? 2 : 3); i++) {
    led(ok ? 0 : LVL, 0, ok ? LVL : 0);
    delay(120);
    led(0, 0, 0);
    delay(90);
  }
  ledIdle();
}

void setup() {
  Serial.begin(115200);
  pixel.begin();
  ledIdle();  // green immediately: Zero is powered and running

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  rf.begin();

  delay(BOOT_WAIT_MS);

  // Mandatory Slim init, boot animation, then rest with player 1 lit.
  bool a = rf.startModule();
  delay(CMD_GAP_MS);
  bool b = rf.bootAnimation();
  delay(CMD_GAP_MS);
  rf.ledPlayer1();
  Serial.printf("init: startModule=%d bootAnimation=%d\n", a, b);
  ledResult(a && b);
}

void loop() {
  static bool wasDown = false;
  static uint32_t downAt = 0;

  bool isDown = (digitalRead(PIN_BUTTON) == LOW);
  uint32_t now = millis();

  if (isDown && !wasDown) {
    wasDown = true;
    downAt = now;
  } else if (!isDown && wasDown) {
    wasDown = false;
    if (now - downAt >= DEBOUNCE_MS) {
      Serial.println("sync");
      led(0, 0, LVL);            // Zero LED blue while syncing
      rf.sweep();                // ring shows the searching sweep
      bool ok = rf.syncController();
      delay(SYNC_WINDOW_MS);     // pairing window (sweep runs)
      rf.ledAllFlash();          // all-four flash = connected
      delay(1200);
      rf.ledPlayer1();           // settle back to player 1
      ledResult(ok);
    }
  }
}
