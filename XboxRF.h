#pragma once
#include <Arduino.h>

// Driver for the Xbox 360 S (Slim / "Boron") FPM control bus.
//
// Protocol (module-clocked, confirmed by the working Slim implementation
// ginokgx/xbox360slimRF):
//   - The MODULE generates the clock on C_CLK. The host only pulls DATA low
//     to start, then sets each data bit synchronized to the module's clock.
//   - The Slim clock is SLOW (order of hundreds of Hz), NOT the phat's
//     ~250 kHz. Timeouts must be generous or transactions look "dead".
//   - Idle: both lines pulled high. Start: host drives DATA low. Per bit:
//     wait for the clock to change, settle ~1 ms into the low phase, set the
//     data bit, wait for the next clock change. MSB first.
//   - The Slim needs a mandatory "start module" command at power-on before
//     LEDs/sync work, and its sync frame is 11 bits (phat's 10 + a trailing
//     1 to snap DATA high at the end).
//
// This does NOT touch the module's USB lines (D+/D-/3V3/GND) — those go
// straight to the PC, and USB enumerates on power alone (not gated by this
// bus).

class XboxRF {
public:
  XboxRF(uint8_t dataPin, uint8_t clkPin);

  // Configure pins (idle: inputs, pulled high).
  void begin();

  // Send a command, MSB-first, synchronized to the module's clock.
  // Returns false if the module's clock stalled (timeout).
  bool sendData(const uint8_t* cmd, uint8_t nbits);

  // --- Named commands ---
  bool startModule();     // mandatory Slim power-on init  (0b0000010010)
  bool bootAnimation();   // ring-of-light boot animation  (0x085)
  bool syncController();  // begin controller binding, 11-bit
  bool sweep();           // rotating "searching" ring animation (0x088)
  bool ledPlayer1();      // light quadrant 1 / player 1 (0x0A1)
  bool ledAllFlash();     // all four quadrants flash — sync confirm (0x0C0)
  bool ledsOff();

  // Slim clock is slow (hundreds of Hz); allow tens of ms per edge.
  static constexpr uint32_t CLK_TIMEOUT_US = 250000;

private:
  uint8_t _data;
  uint8_t _clk;
  bool waitClockChange(int prevLevel, uint32_t timeoutUs);
  void releaseData();
};
