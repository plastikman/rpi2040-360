#include "XboxRF.h"

// Command frames for the Xbox 360 S (Boron) FPM two-wire bus.
//
// Each frame is 10 bits MSB-first: bits[0] = ACK bit (0 = host->FPM),
// bits[1..9] = the 9 command bits documented by drtrinity.
//
// drtrinity notation -> 9 command bits -> full 10-bit frame (ACK prefixed):
//   Start binding    0000000100  (already 10-bit, ACK=0)   [VERIFIED]
//   All LEDs off     010000000   -> {0, 010000000}         [reconstructed]
//   Power LED on     01000010 X  -> {0, 01000010 0}        [reconstructed]
//   Power LED off    01000100 X  -> {0, 01000100 0}        [reconstructed]
//   Power LED blink  01000110 X  -> {0, 01000110 0}        [reconstructed]
//   Exit error state 010010000   -> {0, 010010000}         [reconstructed]
//   Wireless toggle  00000100 X  -> {0, 00000100 X}        [reconstructed]
//
// The [reconstructed] frames follow the blog's bit notation but the X-bit
// meaning and exact alignment are unconfirmed — verify with a logic
// analyzer before trusting them.

static const uint8_t CMD_SYNC[10]          = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0};
static const uint8_t CMD_ALL_LEDS_OFF[10]  = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0};
static const uint8_t CMD_PWR_LED_ON[10]    = {0, 0, 1, 0, 0, 0, 0, 1, 0, 0};
static const uint8_t CMD_PWR_LED_OFF[10]   = {0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
static const uint8_t CMD_PWR_LED_BLINK[10] = {0, 0, 1, 0, 0, 0, 1, 1, 0, 0};
static const uint8_t CMD_EXIT_ERROR[10]    = {0, 0, 1, 0, 0, 1, 0, 0, 0, 0};
static const uint8_t CMD_WIRELESS_ON[10]   = {0, 0, 0, 0, 0, 0, 1, 0, 0, 1};
static const uint8_t CMD_WIRELESS_OFF[10]  = {0, 0, 0, 0, 0, 0, 1, 0, 0, 0};

XboxRF::XboxRF(uint8_t dataPin, uint8_t clkPin)
    : _data(dataPin), _clk(clkPin) {}

void XboxRF::begin() {
  // Idle: both lines released to INPUT, held HIGH. Internal pull-ups are a
  // backstop; a salvaged FPM needs external ~10k pull-ups on C_DATA/C_CLK
  // (the console's 100k pull-ups live on the motherboard, not the module).
  pinMode(_clk, INPUT_PULLUP);
  pinMode(_data, INPUT_PULLUP);
}

void XboxRF::releaseBus() {
  digitalWrite(_data, HIGH);
  pinMode(_data, INPUT_PULLUP);
}

// Block until CLK reaches `level`, or until timeoutUs elapses.
// Returns true on the edge, false on timeout.
bool XboxRF::waitClock(bool level, uint32_t timeoutUs) {
  uint32_t start = micros();
  int want = level ? HIGH : LOW;
  while (digitalRead(_clk) != want) {
    if (micros() - start > timeoutUs) return false;
  }
  return true;
}

bool XboxRF::sendCommand(const uint8_t bits[10]) {
  // Request a transaction: pull DATA low while CLK is (idle) high. This also
  // presents ACK bit 0 (host->FPM). The FPM then starts clocking at 250 kHz.
  pinMode(_data, OUTPUT);
  digitalWrite(_data, LOW);

  for (int i = 0; i < 10; i++) {
    // Set the bit up while CLK is high...
    if (!waitClock(true, CLK_TIMEOUT_US)) {
      releaseBus();
      return false;
    }
    digitalWrite(_data, bits[i] ? HIGH : LOW);

    // ...the FPM samples it on the falling edge (CLK goes low).
    if (!waitClock(false, CLK_TIMEOUT_US)) {
      releaseBus();
      return false;
    }
  }

  // End: both lines return high.
  releaseBus();
  return true;
}

bool XboxRF::syncController() { return sendCommand(CMD_SYNC); }
bool XboxRF::allLedsOff()     { return sendCommand(CMD_ALL_LEDS_OFF); }
bool XboxRF::powerLedOn()     { return sendCommand(CMD_PWR_LED_ON); }
bool XboxRF::powerLedOff()    { return sendCommand(CMD_PWR_LED_OFF); }
bool XboxRF::powerLedBlink()  { return sendCommand(CMD_PWR_LED_BLINK); }
bool XboxRF::exitErrorState() { return sendCommand(CMD_EXIT_ERROR); }

bool XboxRF::wirelessToggle(bool on) {
  return sendCommand(on ? CMD_WIRELESS_ON : CMD_WIRELESS_OFF);
}
