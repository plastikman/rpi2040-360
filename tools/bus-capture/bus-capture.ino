// Bus capture / mini logic-analyzer for the Xbox 360 FPM control bus.
//
// TRIGGERED mode: the Zero sits armed watching CLK (GP4) and DATA (GP3).
// The instant either line moves, it records every transition until the bus
// goes quiet (~200 ms), then dumps the list. It re-arms automatically, so
// you can leave it running and just power-cycle the MODULE to catch its
// power-on handshake.
//
// Output goes to BOTH:
//   - Serial  (USB-C CDC), and
//   - Serial1 (hardware UART0, TX on GP0) at 115200 8N1 — wire a USB-serial
//     adapter: adapter RX <- Zero GP0, adapter GND <- Zero GND, 3.3V logic.
//
// USE:
//   1. Flash this. Power the Zero (USB-C or 5V pad). Open serial at 115200.
//   2. LED GREEN = armed and waiting.
//   3. Power-cycle the MODULE (drop its 3V3 and bring it back) so the FPM
//      re-runs its power-on sequence. LED YELLOW = capturing, BLUE = dumping.
//   4. Copy the dump between "=== capture ... ===" and "=== end ===".
//
// You can also just re-power the whole thing; the trigger catches the burst
// whenever it starts.

#include <Adafruit_NeoPixel.h>

constexpr uint8_t PIN_DATA = 3;
constexpr uint8_t PIN_CLK  = 4;
constexpr uint8_t PIN_RGB  = 16;
constexpr uint8_t LVL      = 24;
constexpr int     MAXEV    = 6000;
constexpr uint32_t QUIET_US = 200000UL;  // 200 ms of no activity ends a burst

Adafruit_NeoPixel pixel(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

uint32_t evT[MAXEV];
uint8_t  evClk[MAXEV];
uint8_t  evData[MAXEV];
int      n = 0;

void led(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void dump() {
  Serial.printf("=== capture: %d transitions (t in us from trigger) ===\n", n);
  Serial1.printf("=== capture: %d transitions (t in us from trigger) ===\n", n);
  for (int i = 0; i < n; i++) {
    Serial.printf("%lu\tCLK=%d\tDATA=%d\n", (unsigned long)evT[i], evClk[i], evData[i]);
    Serial1.printf("%lu\tCLK=%d\tDATA=%d\n", (unsigned long)evT[i], evClk[i], evData[i]);
  }
  Serial.println("=== end ===");
  Serial1.println("=== end ===");
}

void setup() {
  pinMode(PIN_DATA, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  pixel.begin();
  Serial.begin(115200);
  Serial1.begin(115200);
  uint32_t w = millis();
  while (!Serial && (millis() - w) < 2000) {}
  Serial.println("bus-capture armed");
  Serial1.println("bus-capture armed");
}

void loop() {
  // --- Armed: wait for the first transition on either line ---
  led(0, LVL, 0);  // green
  int lastClk = digitalRead(PIN_CLK);
  int lastData = digitalRead(PIN_DATA);
  while (digitalRead(PIN_CLK) == lastClk && digitalRead(PIN_DATA) == lastData) {}

  // --- Triggered: record until the bus goes quiet ---
  led(LVL, LVL, 0);  // yellow
  n = 0;
  uint32_t start = micros();
  uint32_t lastEdge = start;
  while (n < MAXEV) {
    int c = digitalRead(PIN_CLK);
    int d = digitalRead(PIN_DATA);
    if (c != lastClk || d != lastData) {
      evT[n] = micros() - start;
      evClk[n] = c;
      evData[n] = d;
      n++;
      lastClk = c;
      lastData = d;
      lastEdge = micros();
    } else if ((micros() - lastEdge) > QUIET_US) {
      break;  // burst finished
    }
  }

  // --- Dump ---
  led(0, 0, LVL);  // blue
  dump();
  delay(1000);
}
