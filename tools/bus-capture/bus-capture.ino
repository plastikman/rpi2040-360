// Bus capture / mini logic-analyzer for the Xbox 360 FPM control bus.
//
// At power-up it records every CLK (GP4) and DATA (GP3) transition for ~4s
// into RAM, then dumps them over the Zero's USB-C serial at 115200 baud.
// The goal is to capture the FPM's power-on handshake (the "blue" burst the
// monitor showed) so the protocol can be decoded.
//
// Output goes to BOTH:
//   - Serial  (USB-C CDC), and
//   - Serial1 (hardware UART0, TX on GP0) at 115200 8N1 — wire a USB-serial
//     adapter: adapter RX <- Zero GP0, adapter GND <- Zero GND, 3.3V logic.
//
// USE:
//   1. Flash this. Power the Zero (USB-C, or 5V pad if reading via the
//      UART adapter). The module runs off the Zero's 3V3.
//   2. Open serial at 115200 (USB-C COM port, or the adapter's COM port).
//   3. POWER-CYCLE the Zero (cut its power fully) so the module's 3V3 drops
//      and the FPM re-runs its power-on sequence. A plain RESET won't do it.
//   4. After the ~4s capture, it prints the transition list on a loop.
//      Copy the whole dump.
//
// RGB: yellow = capturing, blue = done/dumping.

#include <Adafruit_NeoPixel.h>

constexpr uint8_t PIN_DATA = 3;
constexpr uint8_t PIN_CLK  = 4;
constexpr uint8_t PIN_RGB  = 16;
constexpr int     MAXEV    = 6000;

Adafruit_NeoPixel pixel(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

uint32_t evT[MAXEV];
uint8_t  evClk[MAXEV];
uint8_t  evData[MAXEV];
int      n = 0;

void setup() {
  pinMode(PIN_DATA, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  pixel.begin();
  pixel.setPixelColor(0, pixel.Color(24, 24, 0));  // yellow: capturing
  pixel.show();

  // Capture any CLK/DATA transition for ~4s, starting immediately at boot
  // so we catch the FPM's power-on clocking.
  uint32_t start = micros();
  int lastClk = digitalRead(PIN_CLK);
  int lastData = digitalRead(PIN_DATA);
  while ((micros() - start) < 4000000UL && n < MAXEV) {
    int c = digitalRead(PIN_CLK);
    int d = digitalRead(PIN_DATA);
    if (c != lastClk || d != lastData) {
      evT[n] = micros() - start;
      evClk[n] = c;
      evData[n] = d;
      n++;
      lastClk = c;
      lastData = d;
    }
  }

  pixel.setPixelColor(0, pixel.Color(0, 0, 24));  // blue: done, dumping
  pixel.show();

  Serial.begin(115200);          // USB-C CDC
  Serial1.begin(115200);         // hardware UART0, TX on GP0
  uint32_t w = millis();
  while (!Serial && (millis() - w) < 3000) {}
}

void loop() {
  Serial.printf("=== capture: %d transitions (t in us since boot) ===\n", n);
  Serial1.printf("=== capture: %d transitions (t in us since boot) ===\n", n);
  for (int i = 0; i < n; i++) {
    Serial.printf("%lu\tCLK=%d\tDATA=%d\n", (unsigned long)evT[i], evClk[i], evData[i]);
    Serial1.printf("%lu\tCLK=%d\tDATA=%d\n", (unsigned long)evT[i], evClk[i], evData[i]);
  }
  Serial.println("=== end ===");
  Serial1.println("=== end ===");
  delay(10000);
}
