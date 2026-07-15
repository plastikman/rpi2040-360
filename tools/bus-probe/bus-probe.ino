// Active bus probe for the Xbox 360 FPM control bus.
//
// Per the protocol, the FPM only generates its 250 kHz clock AFTER the SMC
// initiates by pulling DATA low. So this probe does exactly that, on a loop:
//
//   1. Release both lines (INPUT_PULLUP) and record their idle levels.
//   2. Pull DATA (GP3) LOW to initiate a transaction.
//   3. Watch CLK (GP4) for ~20 ms and record every edge.
//   4. Release DATA, report the result, wait, repeat.
//
// This directly answers: does the FPM respond to an SMC-initiated start?
//   - CLK edges seen  -> FPM is clocking; we can decode/fix the protocol.
//   - No CLK edges     -> FPM ignores our initiation (idle-state, timing, or
//                         it needs something we're not providing).
//
// Output on Serial (USB-C) AND Serial1 (UART0 TX=GP0, 115200). RGB: blue =
// FPM responded (edges), red = no response, green between tries.

#include <Adafruit_NeoPixel.h>

constexpr uint8_t PIN_DATA = 3;
constexpr uint8_t PIN_CLK  = 4;
constexpr uint8_t PIN_RGB  = 16;
constexpr uint8_t LVL      = 24;
constexpr int     MAXEV    = 4000;
constexpr uint32_t WINDOW_US = 20000UL;  // watch CLK for 20 ms after initiating

Adafruit_NeoPixel pixel(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
uint32_t evT[MAXEV];
uint8_t  evClk[MAXEV];
int      n = 0;

void led(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void say2(const char* s) { Serial.print(s); Serial1.print(s); }

void setup() {
  pinMode(PIN_DATA, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  pixel.begin();
  Serial.begin(115200);
  Serial1.begin(115200);
  led(0, LVL, 0);
  say2("bus-probe ready\n");
}

void loop() {
  // 1. Idle: release both lines, let them settle, sample levels.
  pinMode(PIN_DATA, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  delay(20);
  int idleClk = digitalRead(PIN_CLK);
  int idleData = digitalRead(PIN_DATA);

  // 2. Initiate: pull DATA low.
  led(LVL, LVL, 0);  // yellow while probing
  pinMode(PIN_DATA, OUTPUT);
  digitalWrite(PIN_DATA, LOW);

  // 3. Watch CLK for edges.
  n = 0;
  uint32_t start = micros();
  int lastClk = digitalRead(PIN_CLK);
  while ((micros() - start) < WINDOW_US && n < MAXEV) {
    int c = digitalRead(PIN_CLK);
    if (c != lastClk) {
      evT[n] = micros() - start;
      evClk[n] = c;
      n++;
      lastClk = c;
    }
  }

  // 4. Release and report.
  pinMode(PIN_DATA, INPUT_PULLUP);

  Serial.printf("idle CLK=%d DATA=%d | after DATA-low: %d CLK edges in 20ms\n", idleClk, idleData, n);
  Serial1.printf("idle CLK=%d DATA=%d | after DATA-low: %d CLK edges in 20ms\n", idleClk, idleData, n);
  for (int i = 0; i < n && i < 40; i++) {  // first 40 edges are plenty
    Serial.printf("  %lu us CLK=%d\n", (unsigned long)evT[i], evClk[i]);
    Serial1.printf("  %lu us CLK=%d\n", (unsigned long)evT[i], evClk[i]);
  }
  say2("---\n");

  led(n > 0 ? 0 : LVL, 0, n > 0 ? LVL : 0);  // blue if responded, red if not
  delay(1500);
}
