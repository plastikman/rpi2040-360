// Bus health monitor for the Xbox 360 FPM control bus.
//
// Continuously attempts a benign control-bus transaction and shows the
// result LIVE, so you can find an intermittent connection by wiggling wires
// and watching the LED flip:
//
//   GREEN = last transaction succeeded (bus is talking)
//   RED   = last transaction timed out (bus not responding)
//
// Also prints a running success/fail tally over Serial (USB-C) and Serial1
// (UART0 TX=GP0) at 115200.
//
// How to read the result:
//   - Flickers GREEN<->RED as you press/wiggle a specific joint  -> that
//     joint is the intermittent connection. Reflow it.
//   - Solid GREEN, drops to RED only when you disturb one wire    -> same.
//   - Solid RED no matter what you touch                          -> not a
//     wiggle/contact issue; the module isn't responding at all right now.
//   - Solid GREEN, stays green                                    -> the bus
//     is reliably working; the earlier trouble was contact while wiring.
//
// Pins match the main firmware: DATA=GP3, CLK=GP4, RGB=GP16.

#include <Adafruit_NeoPixel.h>

constexpr uint8_t PIN_DATA = 3;
constexpr uint8_t PIN_CLK  = 4;
constexpr uint8_t PIN_RGB  = 16;
constexpr uint8_t LVL      = 28;
constexpr uint32_t EDGE_TIMEOUT_US = 5000;  // snappy: 5 ms per clock edge

// Benign command to send repeatedly (all LEDs off): ACK=0 + 010000000.
const uint8_t CMD[10] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0};

Adafruit_NeoPixel pixel(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
uint32_t okCount = 0, failCount = 0;

bool waitClock(bool level, uint32_t timeoutUs) {
  uint32_t s = micros();
  int want = level ? HIGH : LOW;
  while (digitalRead(PIN_CLK) != want) {
    if (micros() - s > timeoutUs) return false;
  }
  return true;
}

bool sendCommand(const uint8_t b[10]) {
  pinMode(PIN_DATA, OUTPUT);
  digitalWrite(PIN_DATA, LOW);
  for (int i = 0; i < 10; i++) {
    if (!waitClock(true, EDGE_TIMEOUT_US)) { pinMode(PIN_DATA, INPUT_PULLUP); return false; }
    digitalWrite(PIN_DATA, b[i] ? HIGH : LOW);
    if (!waitClock(false, EDGE_TIMEOUT_US)) { pinMode(PIN_DATA, INPUT_PULLUP); return false; }
  }
  digitalWrite(PIN_DATA, HIGH);
  pinMode(PIN_DATA, INPUT_PULLUP);
  return true;
}

void setup() {
  pinMode(PIN_DATA, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  pixel.begin();
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial.println("bus-health running");
  Serial1.println("bus-health running");
}

void loop() {
  static uint32_t lastReport = 0;

  bool ok = sendCommand(CMD);
  if (ok) okCount++; else failCount++;

  // Live LED: green on success, red on failure.
  if (ok) pixel.setPixelColor(0, pixel.Color(0, LVL, 0));
  else    pixel.setPixelColor(0, pixel.Color(LVL, 0, 0));
  pixel.show();

  uint32_t now = millis();
  if (now - lastReport > 500) {
    lastReport = now;
    uint32_t total = okCount + failCount;
    uint32_t pct = total ? (okCount * 100) / total : 0;
    Serial.printf("ok=%lu fail=%lu  (%lu%% success)\n", okCount, failCount, pct);
    Serial1.printf("ok=%lu fail=%lu  (%lu%% success)\n", okCount, failCount, pct);
  }

  delay(60);
}
