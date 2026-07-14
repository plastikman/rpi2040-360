#pragma once
#include <Arduino.h>

// Driver for the Xbox 360 S (Slim / "Boron") Front Panel Module control bus.
//
// The FPM talks to the console SMC over a proprietary 2-wire synchronous
// serial bus: C_DATA + C_CLK. The FPM is the clock master: after the host
// (SMC — here, this RP2040) pulls DATA low to start, the FPM generates a
// ~250 kHz clock and the host clocks out the frame.
//
// Frame: 10 bits, MSB-first, sampled on the CLK FALLING edge.
//   bit 0  = ACK/direction bit: 0 = host->FPM (what we send), 1 = FPM->host
//   bits 1..9 = 9 command bits
//
// Transaction:
//   1. Idle: DATA and CLK float HIGH (external pull-ups; motherboard used
//      100k, a salvaged board needs its own ~10k on C_DATA and C_CLK).
//   2. Host pulls DATA LOW as OUTPUT to request (also presents ACK bit 0).
//   3. FPM drives CLK at 250 kHz, 50% duty. Host sets each data bit while
//      CLK is HIGH; the FPM samples it on the falling edge.
//   4. Transaction ends when both CLK and DATA return HIGH.
//
// This does NOT touch the module's USB lines (D+/D-/3V3/GND) — those go
// straight from the FPM connector to the PC. This driver only controls the
// LEDs and triggers controller binding (sync).
//
// Protocol + command table source (2025 reverse engineering):
//   https://drtrinity.uk/blog/2025/05/12/reversing-the-rf-board-1
//
// STATUS: mechanics + SYNC command are solid; the LED/toggle command frames
// are reconstructed from the blog's notation and need logic-analyzer
// verification against real hardware.

class XboxRF {
public:
  XboxRF(uint8_t dataPin, uint8_t clkPin);

  // Configure pins. Call once from setup().
  void begin();

  // Send a raw 10-bit frame (MSB-first, bits[0] = ACK bit = 0 for host->FPM).
  // Returns false if the FPM clock stalled (timeout) — module unpowered,
  // not booted yet, or bus miswired.
  bool sendCommand(const uint8_t bits[10]);

  // --- Named commands ---
  bool syncController();   // begin controller binding (verified frame)
  bool allLedsOff();       // [reconstructed]
  bool powerLedOn();       // [reconstructed]
  bool powerLedOff();      // [reconstructed]
  bool powerLedBlink();    // [reconstructed]
  bool exitErrorState();   // [reconstructed]
  bool wirelessToggle(bool on);  // enable/disable radio [reconstructed]

  // The bus runs ~250 kHz (2 us half-period). Real edges arrive within
  // microseconds; 50 ms is a generous stall guard against a dead bus.
  static constexpr uint32_t CLK_TIMEOUT_US = 50000;

private:
  uint8_t _data;
  uint8_t _clk;
  bool waitClock(bool level, uint32_t timeoutUs);
  void releaseBus();
};
