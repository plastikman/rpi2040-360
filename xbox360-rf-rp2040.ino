// Xbox 360 S (Slim / "Boron") FPM -> USB desktop adapter, RP2040-Zero edition.
//
// The RP2040 replaces the console SMC's role toward the Front Panel Module.
// It is NOT in the USB data path: the FPM's USB lines (D+/D-) run straight to
// the PC. This firmware only:
//   - reads a sync button
//   - drives the power LED / triggers controller binding over the FPM's
//     2-wire control bus (see XboxRF.cpp)
//   - shows status on the RP2040-Zero's onboard WS2812 RGB LED (GP16)
//
// Onboard RGB status (WS2812 on GP16):
//   solid green  -> powered and running (shows immediately on power-up)
//   blue flash   -> sync command sent OK
//   cyan flash   -> wireless toggle sent OK
//   red flash    -> control-bus timeout (FPM not responding)
//
// Button behaviour:
//   short press (< LONG_PRESS_MS)  -> start controller binding (sync)
//   long  press (>= LONG_PRESS_MS) -> toggle the wireless radio off
//
// Board: Waveshare RP2040-Zero   FQBN: rp2040:rp2040:waveshare_rp2040_zero
// FPM connector pinout: see docs/wiring.md

#include <Adafruit_NeoPixel.h>
#include "XboxRF.h"

// --- Pin map (RP2040-Zero GPIO numbers) ---
constexpr uint8_t PIN_BUTTON = 2;   // sync button to GND (uses internal pull-up)
constexpr uint8_t PIN_DATA   = 3;   // FPM pin 2  C_DATA (+ pull-up)
constexpr uint8_t PIN_CLK    = 4;   // FPM pin 3  C_CLK  (+ pull-up)
constexpr uint8_t PIN_RGB    = 16;  // onboard WS2812

// --- Timing ---
constexpr uint16_t DEBOUNCE_MS   = 30;
constexpr uint16_t LONG_PRESS_MS = 1500;
constexpr uint16_t BOOT_WAIT_MS  = 3500;  // let the FPM boot before talking

// --- Status colours (dim; WS2812 is very bright) ---
constexpr uint8_t LVL = 32;  // brightness cap for colour channels

XboxRF rf(PIN_DATA, PIN_CLK);
Adafruit_NeoPixel pixel(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
bool radioOn = true;

void led(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void ledIdle() { led(0, LVL, 0); }  // green = alive

// Flash a colour a few times, then return to the green idle state.
void ledFlash(uint8_t r, uint8_t g, uint8_t b, uint8_t times) {
  for (uint8_t i = 0; i < times; i++) {
    led(r, g, b);
    delay(120);
    led(0, 0, 0);
    delay(90);
  }
  ledIdle();
}

// Reflect a command result: blue/cyan flash on success, red on timeout.
void ledResult(bool ok, uint8_t r, uint8_t g, uint8_t b) {
  if (ok) ledFlash(r, g, b, 2);
  else    ledFlash(LVL, 0, 0, 3);  // red = FPM didn't answer the bus
}

void setup() {
  Serial.begin(115200);

  pixel.begin();
  pixel.setBrightness(255);
  ledIdle();  // green immediately: proves the Zero is powered and running

  pinMode(PIN_BUTTON, INPUT_PULLUP);  // pressed = LOW
  rf.begin();

  delay(BOOT_WAIT_MS);  // FPM needs time to come up after power

  rf.powerLedOn();  // light the module's own power LED (best-effort)

  Serial.println("xbox360-rf-rp2040 (Boron FPM) ready");
}

void loop() {
  static bool wasDown = false;
  static uint32_t downAt = 0;
  static bool longFired = false;

  bool isDown = (digitalRead(PIN_BUTTON) == LOW);
  uint32_t now = millis();

  if (isDown && !wasDown) {
    // press begins
    wasDown = true;
    downAt = now;
    longFired = false;
  } else if (isDown && wasDown) {
    // held: fire the long-press action once past the threshold
    if (!longFired && (now - downAt) >= LONG_PRESS_MS) {
      longFired = true;
      radioOn = !radioOn;
      Serial.printf("long press -> wireless %s\n", radioOn ? "on" : "off");
      bool ok = rf.wirelessToggle(radioOn);
      ledResult(ok, 0, LVL, LVL);  // cyan on success
    }
  } else if (!isDown && wasDown) {
    // release
    wasDown = false;
    uint32_t held = now - downAt;
    if (held >= DEBOUNCE_MS && !longFired) {
      Serial.println("short press -> sync");
      bool ok = rf.syncController();
      ledResult(ok, 0, 0, LVL);  // blue on success
    }
  }
}
