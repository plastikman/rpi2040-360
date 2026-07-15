// Bus monitor / diagnostic firmware for the Xbox 360 FPM control bus.
//
// This does NOT drive the bus. It passively watches C_DATA (GP3) and C_CLK
// (GP4) and reports, once per second:
//   - the current logic level the RP2040 reads on each line
//   - how many edges/second it sees (is the FPM clocking on its own?)
//   - what % of the time CLK is high
//
// It also summarises CLK behaviour on the onboard WS2812 (GP16):
//   GREEN  = CLK idle HIGH        -> healthy idle; problem is our send logic
//   RED    = CLK stuck LOW        -> module is holding the clock low / not clocking
//   BLUE   = CLK toggling         -> FPM is clocking on its own (protocol is FPM-driven)
//
// Read the serial output over the Zero's USB-C at 115200 baud.
// Pins match the main firmware: DATA=GP3, CLK=GP4, button=GP2, RGB=GP16.

#include <Adafruit_NeoPixel.h>

constexpr uint8_t PIN_DATA = 3;
constexpr uint8_t PIN_CLK  = 4;
constexpr uint8_t PIN_BTN  = 5;  // moved off GP2
constexpr uint8_t PIN_RGB  = 16;
constexpr uint8_t LVL      = 24;

Adafruit_NeoPixel pixel(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

void led(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_DATA, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_BTN, INPUT_PULLUP);
  pixel.begin();
  led(LVL, LVL, LVL);  // white while starting
  delay(400);
}

void loop() {
  uint32_t t0 = millis();
  uint32_t clkEdges = 0, dataEdges = 0, clkHigh = 0, samples = 0;
  int lastClk = digitalRead(PIN_CLK);
  int lastData = digitalRead(PIN_DATA);

  // Sample as fast as we can for ~1 second.
  while (millis() - t0 < 1000) {
    int c = digitalRead(PIN_CLK);
    int d = digitalRead(PIN_DATA);
    if (c != lastClk) { clkEdges++; lastClk = c; }
    if (d != lastData) { dataEdges++; lastData = d; }
    if (c) clkHigh++;
    samples++;
  }

  int clkLvl = digitalRead(PIN_CLK);
  int dataLvl = digitalRead(PIN_DATA);
  uint32_t highPct = samples ? (clkHigh * 100) / samples : 0;

  Serial.printf("CLK lvl=%d edges/s=%lu high=%lu%%  |  DATA lvl=%d edges/s=%lu  |  btn=%d\n",
                clkLvl, clkEdges, highPct, dataLvl, dataEdges,
                digitalRead(PIN_BTN) == LOW ? 1 : 0);

  if (clkEdges > 10)       led(0, 0, LVL);   // blue: CLK toggling on its own
  else if (clkLvl == HIGH) led(0, LVL, 0);   // green: CLK idle high
  else                     led(LVL, 0, 0);   // red: CLK stuck low
}
