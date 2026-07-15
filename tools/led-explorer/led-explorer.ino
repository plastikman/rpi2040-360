// LED command explorer for the Xbox 360 S (Boron) FPM ring-of-light.
//
// Each press of the sync button (GP2) sends the NEXT candidate LED command
// over the control bus and prints its index/label/hex over Serial + Serial1
// (UART0 TX=GP0), both at 115200. Watch the module's ring and note which
// index makes it SPIN (searching animation) and which lights only QUADRANT 1.
// Report those indices and they get baked into the main firmware.
//
// Protocol matches the main driver (ginokgx/xbox360slimRF): module-clocked,
// slow clock, data set ~1 ms into the clock-low phase, MSB-first.
// Pins: DATA=GP3, CLK=GP4, button=GP2, RGB=GP16.

#include <Adafruit_NeoPixel.h>

constexpr uint8_t PIN_BUTTON = 2;
constexpr uint8_t PIN_DATA   = 3;
constexpr uint8_t PIN_CLK    = 4;
constexpr uint8_t PIN_RGB    = 16;
constexpr uint8_t LVL        = 28;
constexpr uint32_t CLK_TIMEOUT_US = 250000;

Adafruit_NeoPixel pixel(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

// Init frames (from the working Slim code).
const uint8_t CMD_START[10] = {0, 0, 0, 0, 0, 1, 0, 0, 1, 0};
const uint8_t CMD_BOOT[10]  = {0, 0, 1, 0, 0, 0, 0, 1, 0, 1};  // 0x085

// Candidate LED frames = ACK bit 0 + 9-bit command (MSB-first).
struct Cand { const char* label; uint8_t bits[10]; };
const Cand CANDS[] = {
  {"0x080 all-off",     {0,0,1,0,0,0,0,0,0,0}},
  {"0x084 LED_INIT",    {0,0,1,0,0,0,0,1,0,0}},
  {"0x085 BOOTANIM",    {0,0,1,0,0,0,0,1,0,1}},
  {"0x086",             {0,0,1,0,0,0,0,1,1,0}},
  {"0x087",             {0,0,1,0,0,0,0,1,1,1}},
  {"0x088",             {0,0,1,0,0,0,1,0,0,0}},
  {"0x08D flash+boot",  {0,0,1,0,0,0,1,1,0,1}},
  {"0x0A1 greenQ b0",   {0,0,1,0,1,0,0,0,0,1}},
  {"0x0A2 greenQ b1",   {0,0,1,0,1,0,0,0,1,0}},
  {"0x0A4 greenQ b2",   {0,0,1,0,1,0,0,1,0,0}},
  {"0x0A8 greenQ b3",   {0,0,1,0,1,0,1,0,0,0}},
  {"0x0AF greenQ all",  {0,0,1,0,1,0,1,1,1,1}},
  {"0x0B1 redQ b0",     {0,0,1,0,1,1,0,0,0,1}},
  {"0x0BF redQ all",    {0,0,1,0,1,1,1,1,1,1}},
  {"0x0C0",             {0,0,1,1,0,0,0,0,0,0}},
  {"0x0C1",             {0,0,1,1,0,0,0,0,0,1}},
  {"0x0D0",             {0,0,1,1,0,1,0,0,0,0}},
  {"0x0E0",             {0,0,1,1,1,0,0,0,0,0}},
  {"0x040",             {0,0,0,1,0,0,0,0,0,0}},
  {"0x041",             {0,0,0,1,0,0,0,0,0,1}},
  {"0x042",             {0,0,0,1,0,0,0,0,1,0}},
  {"0x043",             {0,0,0,1,0,0,0,0,1,1}},
};
const int NCANDS = sizeof(CANDS) / sizeof(CANDS[0]);
int idx = -1;

bool waitClockChange(int prev, uint32_t to) {
  uint32_t s = micros();
  while (digitalRead(PIN_CLK) == prev) { if (micros() - s > to) return false; }
  return true;
}

bool sendData(const uint8_t* cmd, uint8_t nbits) {
  pinMode(PIN_DATA, OUTPUT);
  digitalWrite(PIN_DATA, LOW);
  int prev = digitalRead(PIN_CLK);
  for (uint8_t i = 0; i < nbits; i++) {
    if (!waitClockChange(prev, CLK_TIMEOUT_US)) { pinMode(PIN_DATA, INPUT_PULLUP); return false; }
    prev = digitalRead(PIN_CLK);
    delayMicroseconds(1000);
    digitalWrite(PIN_DATA, cmd[i] ? HIGH : LOW);
    if (!waitClockChange(prev, CLK_TIMEOUT_US)) { pinMode(PIN_DATA, INPUT_PULLUP); return false; }
    prev = digitalRead(PIN_CLK);
  }
  digitalWrite(PIN_DATA, HIGH);
  pinMode(PIN_DATA, INPUT_PULLUP);
  return true;
}

void say(const char* s) { Serial.print(s); Serial1.print(s); }

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_DATA, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  pixel.begin();
  pixel.setPixelColor(0, pixel.Color(0, LVL, 0));
  pixel.show();
  delay(3500);
  sendData(CMD_START, 10); delay(50);
  sendData(CMD_BOOT, 10);
  say("led-explorer ready: press the button to step through LED commands\n");
}

void loop() {
  static bool wasDown = false;
  bool isDown = (digitalRead(PIN_BUTTON) == LOW);

  if (isDown && !wasDown) {
    wasDown = true;
    idx = (idx + 1) % NCANDS;
    bool ok = sendData(CANDS[idx].bits, 10);
    char buf[80];
    snprintf(buf, sizeof(buf), "[%d] %s  sent=%d\n", idx, CANDS[idx].label, ok);
    say(buf);
    // brief blue blink to acknowledge the press
    pixel.setPixelColor(0, pixel.Color(0, 0, LVL)); pixel.show();
    delay(120);
    pixel.setPixelColor(0, pixel.Color(0, LVL, 0)); pixel.show();
  } else if (!isDown && wasDown) {
    wasDown = false;
    delay(30);  // debounce
  }
}
