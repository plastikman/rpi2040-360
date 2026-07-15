#include "XboxRF.h"

// Command frames (MSB-first), from the working Slim implementation
// ginokgx/xbox360slimRF (https://github.com/ginokgx/xbox360slimRF).
//
//   START (0b0000010010) — mandatory Slim "start module" init; without it
//                          the LEDs/sync don't work.
//   BOOT  (0b0010000101) — ring-of-light boot animation (= 0x085).
//   SYNC  (0b00000001001) — phat sync 0b0000000100 + a trailing 1; 11 bits.
//   OFF   (0b0010000000) — LEDs off.
// LED codes verified on-hardware with tools/led-explorer:
//   0x088 = rotating "searching" sweep
//   0x0A1 = quadrant 1 lit (player 1)
//   0x0C0 = all four quadrants flash (sync-complete confirm)
static const uint8_t CMD_START[10]    = {0, 0, 0, 0, 0, 1, 0, 0, 1, 0};
static const uint8_t CMD_BOOT[10]     = {0, 0, 1, 0, 0, 0, 0, 1, 0, 1};  // 0x085
static const uint8_t CMD_SYNC[11]     = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1};
static const uint8_t CMD_SWEEP[10]    = {0, 0, 1, 0, 0, 0, 1, 0, 0, 0};  // 0x088
static const uint8_t CMD_PLAYER1[10]  = {0, 0, 1, 0, 1, 0, 0, 0, 0, 1};  // 0x0A1
static const uint8_t CMD_ALLFLASH[10] = {0, 0, 1, 1, 0, 0, 0, 0, 0, 0};  // 0x0C0
static const uint8_t CMD_OFF[10]      = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0};

XboxRF::XboxRF(uint8_t dataPin, uint8_t clkPin)
    : _data(dataPin), _clk(clkPin) {}

void XboxRF::begin() {
  // Idle: both lines released, held high. External ~1k pull-ups recommended;
  // the module (not this MCU) drives CLK.
  pinMode(_clk, INPUT_PULLUP);
  pinMode(_data, INPUT_PULLUP);
}

void XboxRF::releaseData() {
  digitalWrite(_data, HIGH);
  pinMode(_data, INPUT_PULLUP);
}

// Block until CLK differs from prevLevel (an edge), or timeout.
bool XboxRF::waitClockChange(int prevLevel, uint32_t timeoutUs) {
  uint32_t start = micros();
  while (digitalRead(_clk) == prevLevel) {
    if (micros() - start > timeoutUs) return false;
  }
  return true;
}

bool XboxRF::sendData(const uint8_t* cmd, uint8_t nbits) {
  // Start: pull DATA low while the (module-driven) clock idles high.
  pinMode(_data, OUTPUT);
  digitalWrite(_data, LOW);

  int prev = digitalRead(_clk);  // usually HIGH at idle
  for (uint8_t i = 0; i < nbits; i++) {
    // Wait for the clock to fall...
    if (!waitClockChange(prev, CLK_TIMEOUT_US)) { releaseData(); return false; }
    prev = digitalRead(_clk);

    // ...settle into the low phase, then present the bit.
    delayMicroseconds(1000);
    digitalWrite(_data, cmd[i] ? HIGH : LOW);

    // Wait for the clock to rise (module latches the bit).
    if (!waitClockChange(prev, CLK_TIMEOUT_US)) { releaseData(); return false; }
    prev = digitalRead(_clk);
  }

  releaseData();
  return true;
}

bool XboxRF::startModule()    { return sendData(CMD_START, 10); }
bool XboxRF::bootAnimation()  { return sendData(CMD_BOOT, 10); }
bool XboxRF::syncController() { return sendData(CMD_SYNC, 11); }
bool XboxRF::sweep()          { return sendData(CMD_SWEEP, 10); }
bool XboxRF::ledPlayer1()     { return sendData(CMD_PLAYER1, 10); }
bool XboxRF::ledAllFlash()    { return sendData(CMD_ALLFLASH, 10); }
bool XboxRF::ledsOff()        { return sendData(CMD_OFF, 10); }
