// SMC-clock test for the Xbox 360 S (Boron) FPM control bus.
//
// Capture evidence showed the FPM does NOT generate a clock (CLK stays high;
// only DATA moves). So on the Slim the SMC/host must GENERATE the clock.
// This firmware does that: the RP2040 drives CLK itself and clocks out
// commands, MSB-first, DATA set while CLK high, FPM sampling on the falling
// edge (per the reverse-engineering notes).
//
// It auto-cycles the module's POWER LED on/off every 2 seconds. WATCH THE
// MODULE'S OWN LED: if it turns on/off in time with the onboard RGB (which
// shows which command is being sent), the protocol is cracked.
//
// Onboard RGB: green = sending "LED on", dim = sending "LED off".
// Pins: DATA=GP3, CLK=GP4, RGB=GP16.

#include <Adafruit_NeoPixel.h>

constexpr uint8_t PIN_DATA = 3;
constexpr uint8_t PIN_CLK  = 4;
constexpr uint8_t PIN_RGB  = 16;
constexpr uint8_t LVL      = 28;
constexpr uint16_t HALF_US = 4;   // clock half-period (~125 kHz); tune later

// Frames: bit0 = ACK (0 = host->FPM), bits 1..9 = command.
const uint8_t CMD_PWR_LED_ON[10]  = {0, 0, 1, 0, 0, 0, 0, 1, 0, 0};
const uint8_t CMD_PWR_LED_OFF[10] = {0, 0, 1, 0, 0, 0, 1, 0, 0, 0};

Adafruit_NeoPixel pixel(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

// Host generates the clock. Idle: CLK high, DATA high (released after).
void sendCommand(const uint8_t bits[10]) {
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  digitalWrite(PIN_CLK, HIGH);
  digitalWrite(PIN_DATA, HIGH);
  delayMicroseconds(20);

  // Start: DATA high->low while CLK high.
  digitalWrite(PIN_DATA, LOW);
  delayMicroseconds(HALF_US);

  for (int i = 0; i < 10; i++) {
    digitalWrite(PIN_DATA, bits[i] ? HIGH : LOW);  // set bit while CLK high
    delayMicroseconds(HALF_US);
    digitalWrite(PIN_CLK, LOW);                    // falling edge -> FPM samples
    delayMicroseconds(HALF_US);
    digitalWrite(PIN_CLK, HIGH);                   // rising edge
    delayMicroseconds(HALF_US);
  }

  // Stop, then release both lines to idle-high.
  digitalWrite(PIN_DATA, HIGH);
  delayMicroseconds(20);
  pinMode(PIN_DATA, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  pinMode(PIN_DATA, INPUT_PULLUP);
  pinMode(PIN_CLK, INPUT_PULLUP);
  pixel.begin();
  delay(1500);  // let the FPM settle
  Serial1.println("smc-clock-test: host-generated clock, cycling power LED");
}

void loop() {
  pixel.setPixelColor(0, pixel.Color(0, LVL, 0));  // green
  pixel.show();
  Serial1.println("-> power LED ON");
  sendCommand(CMD_PWR_LED_ON);
  delay(2000);

  pixel.setPixelColor(0, pixel.Color(2, 2, 2));    // dim
  pixel.show();
  Serial1.println("-> power LED OFF");
  sendCommand(CMD_PWR_LED_OFF);
  delay(2000);
}
