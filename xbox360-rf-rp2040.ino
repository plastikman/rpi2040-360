// Xbox 360 RF module -> USB desktop adapter, RP2040-Zero edition.
//
// The RP2040 replaces the original ATtiny25 "front panel" controller. It is
// NOT in the USB data path: the module's USB lines run straight to the PC.
// This firmware only:
//   - reads the physical sync button
//   - drives the LED ring / triggers sync + power-off over the module's
//     2-wire control bus (see XboxRF.cpp)
//
// Button behaviour:
//   short press (< LONG_PRESS_MS)  -> start controller pairing (sync)
//   long  press (>= LONG_PRESS_MS) -> power off connected controllers
//
// Board: Waveshare RP2040-Zero   FQBN: rp2040:rp2040:waveshare_rp2040_zero

#include "XboxRF.h"

// --- Pin map (RP2040-Zero GPIO numbers) ---
constexpr uint8_t PIN_BUTTON = 2;  // sync button to GND (uses internal pull-up)
constexpr uint8_t PIN_DATA   = 3;  // module control-bus DATA (+10k pull-up)
constexpr uint8_t PIN_CLK    = 4;  // module control-bus CLK  (+10k pull-up)

// --- Timing ---
constexpr uint16_t DEBOUNCE_MS   = 30;
constexpr uint16_t LONG_PRESS_MS = 1500;
constexpr uint16_t CMD_GAP_MS    = 50;  // spacing between module commands

XboxRF rf(PIN_DATA, PIN_CLK);

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BUTTON, INPUT_PULLUP);  // pressed = LOW
  rf.begin();

  delay(100);  // let the module settle after power-up

  // Bring the ring to life, then play the boot animation.
  rf.ledInit();
  delay(CMD_GAP_MS);
  rf.startupAnimation();

  Serial.println("xbox360-rf-rp2040 ready");
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
    // held: fire the long-press action once the threshold passes
    if (!longFired && (now - downAt) >= LONG_PRESS_MS) {
      longFired = true;
      Serial.println("long press -> power off controllers");
      rf.powerOffControllers();
    }
  } else if (!isDown && wasDown) {
    // release
    wasDown = false;
    uint32_t held = now - downAt;
    if (held >= DEBOUNCE_MS && !longFired) {
      Serial.println("short press -> sync");
      rf.sync();
    }
  }
}
