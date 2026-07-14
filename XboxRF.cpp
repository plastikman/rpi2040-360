#include "XboxRF.h"

// Command table (10-bit, MSB-first), from the community reverse engineering
// of the Xbox 360 RF module control bus.
static const uint8_t CMD_LED_INIT[10]     = {0, 0, 1, 0, 0, 0, 0, 1, 0, 0};
static const uint8_t CMD_STARTUP_ANIM[10] = {0, 0, 1, 0, 0, 0, 0, 1, 0, 1};
static const uint8_t CMD_SYNC[10]         = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0};
static const uint8_t CMD_POWER_OFF[10]    = {0, 0, 0, 0, 0, 0, 1, 0, 0, 1};
static const uint8_t CMD_LEDS_OFF[10]     = {0, 0, 1, 0, 1, 1, 0, 0, 0, 0};
static const uint8_t CMD_LED_1[10]        = {0, 0, 1, 0, 1, 1, 1, 0, 0, 0};
static const uint8_t CMD_LED_12[10]       = {0, 0, 1, 0, 1, 1, 1, 1, 0, 0};
static const uint8_t CMD_LED_123[10]      = {0, 0, 1, 0, 1, 1, 1, 1, 0, 1};
static const uint8_t CMD_LED_ALL[10]      = {0, 0, 1, 0, 1, 1, 1, 1, 1, 1};

XboxRF::XboxRF(uint8_t dataPin, uint8_t clkPin)
    : _data(dataPin), _clk(clkPin) {}

void XboxRF::begin() {
  // Idle state: both lines released to INPUT. Internal pull-ups are enabled
  // as a backstop, but external 10k pull-ups on DATA and CLK are recommended
  // (the RP2040's internal ~50k is weak for this bus).
  pinMode(_clk, INPUT_PULLUP);
  pinMode(_data, INPUT_PULLUP);
}

void XboxRF::releaseData() {
  digitalWrite(_data, HIGH);
  pinMode(_data, INPUT_PULLUP);
}

// Block until CLK reaches `level`, or until timeoutUs elapses.
// Returns true on the edge, false on timeout.
bool XboxRF::waitClock(bool level, uint32_t timeoutUs) {
  uint32_t start = micros();
  while (digitalRead(_clk) != (level ? HIGH : LOW)) {
    if (micros() - start > timeoutUs) return false;
  }
  return true;
}

bool XboxRF::sendCommand(const uint8_t bits[10]) {
  // Request a transaction: pull DATA low while CLK is (idle) high.
  pinMode(_data, OUTPUT);
  digitalWrite(_data, LOW);

  for (int i = 0; i < 10; i++) {
    // Module drives CLK low -> we set the data bit while it's low...
    if (!waitClock(false, CLK_TIMEOUT_US)) {
      releaseData();
      return false;
    }
    digitalWrite(_data, bits[i] ? HIGH : LOW);

    // ...module raises CLK and samples the bit on the rising edge.
    if (!waitClock(true, CLK_TIMEOUT_US)) {
      releaseData();
      return false;
    }
  }

  releaseData();
  return true;
}

bool XboxRF::ledInit()            { return sendCommand(CMD_LED_INIT); }
bool XboxRF::startupAnimation()   { return sendCommand(CMD_STARTUP_ANIM); }
bool XboxRF::sync()               { return sendCommand(CMD_SYNC); }
bool XboxRF::powerOffControllers(){ return sendCommand(CMD_POWER_OFF); }
bool XboxRF::ledsOff()            { return sendCommand(CMD_LEDS_OFF); }

bool XboxRF::ledPlayers(uint8_t count) {
  switch (count) {
    case 0:  return sendCommand(CMD_LEDS_OFF);
    case 1:  return sendCommand(CMD_LED_1);
    case 2:  return sendCommand(CMD_LED_12);
    case 3:  return sendCommand(CMD_LED_123);
    default: return sendCommand(CMD_LED_ALL);  // 4+
  }
}
