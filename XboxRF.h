#pragma once
#include <Arduino.h>

// Driver for the Xbox 360 wireless RF module's control bus.
//
// The module talks over a proprietary, I2C-like 2-wire synchronous serial
// bus (DATA + CLK). The MODULE is the clock master: it drives CLK, the MCU
// only reads it. A command is 10 bits, sent MSB-first (array index 0..9).
//
// Transaction:
//   1. Idle: DATA and CLK float HIGH (external 10k pull-ups).
//   2. MCU pulls DATA LOW as OUTPUT to request a transaction.
//   3. For each of 10 bits: wait for CLK low, drive the data bit, wait for
//      CLK high (module samples the bit on the rising edge).
//   4. MCU releases DATA back to INPUT; the pull-up returns it to idle HIGH.
//
// This does NOT touch the module's USB lines (D+/D-/VBUS/GND) — those go
// straight from the USB jack to the module and on to the PC. This driver
// only controls the LED ring and triggers controller sync / power-off.

class XboxRF {
public:
  XboxRF(uint8_t dataPin, uint8_t clkPin);

  // Configure pins. Call once from setup().
  void begin();

  // Send a raw 10-bit command (MSB-first, bits[0]..bits[9]).
  // Returns false if the module's clock stalled (timeout) — the module
  // may be unpowered or not yet ready.
  bool sendCommand(const uint8_t bits[10]);

  // --- Named commands (from reverse-engineered command table) ---
  bool ledInit();            // initialise the LED ring
  bool startupAnimation();   // Xbox boot animation
  bool sync();               // begin controller pairing
  bool powerOffControllers();
  bool ledsOff();
  bool ledPlayers(uint8_t count);  // light 0..4 player quadrants

  // Longest a single clock half-period is allowed to take before we give
  // up on a transaction. The bus runs ~330 Hz, so real edges arrive within
  // a few ms; 50 ms is a generous stall guard.
  static constexpr uint32_t CLK_TIMEOUT_US = 50000;

private:
  uint8_t _data;
  uint8_t _clk;
  bool waitClock(bool level, uint32_t timeoutUs);
  void releaseData();
};
