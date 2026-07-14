// Xbox 360 S (Slim / "Boron") FPM -> USB desktop adapter, RP2040-Zero edition.
//
// The RP2040 replaces the console SMC's role toward the Front Panel Module.
// It is NOT in the USB data path: the FPM's USB lines (D+/D-) run straight to
// the PC. This firmware only:
//   - reads a sync button
//   - drives the power LED / triggers controller binding over the FPM's
//     2-wire control bus (see XboxRF.cpp)
//
// Button behaviour:
//   short press (< LONG_PRESS_MS)  -> start controller binding (sync)
//   long  press (>= LONG_PRESS_MS) -> toggle the wireless radio off
//
// Board: Waveshare RP2040-Zero   FQBN: rp2040:rp2040:waveshare_rp2040_zero
// FPM connector pinout: see docs/wiring.md

#include "XboxRF.h"

// --- Pin map (RP2040-Zero GPIO numbers) ---
constexpr uint8_t PIN_BUTTON = 2;  // sync button to GND (uses internal pull-up)
constexpr uint8_t PIN_DATA   = 3;  // FPM pin 2  C_DATA (+10k pull-up)
constexpr uint8_t PIN_CLK    = 4;  // FPM pin 3  C_CLK  (+10k pull-up)

// --- Timing ---
constexpr uint16_t DEBOUNCE_MS   = 30;
constexpr uint16_t LONG_PRESS_MS = 1500;
constexpr uint16_t CMD_GAP_MS    = 50;    // spacing between commands
constexpr uint16_t BOOT_WAIT_MS  = 3500;  // let the FPM boot before talking

XboxRF rf(PIN_DATA, PIN_CLK);
bool radioOn = true;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BUTTON, INPUT_PULLUP);  // pressed = LOW
  rf.begin();

  delay(BOOT_WAIT_MS);  // FPM needs time to come up after power

  // Light the power LED so we can see the module is alive.
  rf.powerLedOn();

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
      rf.wirelessToggle(radioOn);
    }
  } else if (!isDown && wasDown) {
    // release
    wasDown = false;
    uint32_t held = now - downAt;
    if (held >= DEBOUNCE_MS && !longFired) {
      Serial.println("short press -> sync");
      rf.syncController();
    }
  }
}
