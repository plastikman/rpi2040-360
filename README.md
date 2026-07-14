# xbox360-rf-rp2040

Firmware to drive a salvaged **Xbox 360 wireless RF module** as a USB PC
receiver, using a **Waveshare RP2040-Zero** in place of the original ATtiny25.

Based on the [Applied Carbon xboxrf mod](https://www.appliedcarbon.org/xboxrf.html),
re-implemented for the RP2040 with the [arduino-pico](https://github.com/earlephilhower/arduino-pico)
core.

## What it does

The RF module is itself a self-contained USB device — its USB lines go
straight to the PC. The RP2040 is **not** in the USB data path. It replaces
the module's "front panel" controller and only:

- reads the physical **sync button**
- drives the LED ring and triggers **sync** / **power-off** over the module's
  proprietary 2-wire, I²C-like control bus (module is the clock master, 10-bit
  MSB-first commands)

### Button behaviour

| Action | Result |
|--------|--------|
| Short press | Start controller pairing (SYNC) |
| Long press (≥1.5 s) | Power off connected controllers |

## Hardware

- Waveshare RP2040-Zero (USB-C, dual-core Cortex-M0+, 2MB flash)
- Salvaged Xbox 360 internal RF module
- 3× 10kΩ pull-up resistors (bus lines)
- Momentary pushbutton
- USB cable (module → PC)

Wiring and power rules: see [`docs/wiring.md`](docs/wiring.md). Short version:
the module's USB VBUS powers the Zero via its `5V` pad, the Zero's `3V3` pad
powers the module logic, and `GP3`/`GP4` are the DATA/CLK control bus.

## Pin map

| RP2040-Zero GPIO | Function |
|------------------|----------|
| `GP2` | Sync button (to GND, internal pull-up) |
| `GP3` | Module DATA (+10k pull-up) |
| `GP4` | Module CLK  (+10k pull-up) |

## Build & flash

Uses the arduino-pico core. FQBN: `rp2040:rp2040:waveshare_rp2040_zero`.

### arduino-cli

```bash
# one-time: add the core
arduino-cli config add board_manager.additional_urls \
  https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
arduino-cli core update-index
arduino-cli core install rp2040:rp2040

# compile
arduino-cli compile --fqbn rp2040:rp2040:waveshare_rp2040_zero .

# flash: hold BOOT, tap RESET so the board mounts as a USB drive (RPI-RP2),
# then either upload or drag-drop the .uf2
arduino-cli upload --fqbn rp2040:rp2040:waveshare_rp2040_zero -p /dev/ttyACM0 .
```

### Arduino IDE

1. Add the board manager URL above (Preferences → Additional Boards URLs).
2. Boards Manager → install "Raspberry Pi Pico/RP2040".
3. Select **Waveshare RP2040 Zero**.
4. Open `xbox360-rf-rp2040.ino`, Upload.

## Protocol notes

The control bus and command codes are from community reverse engineering.
Command table lives in [`XboxRF.cpp`](XboxRF.cpp).

- [agarmash.com writeup](https://agarmash.com/posts/xbox-360-controller-receiver/)
- [Arduino reference gist](https://gist.github.com/TNKSoftware/f41c233eb52ba76aecffa74f25bdcf04)
- [tkkrlab wiki — XBOX 360 RF Module](https://tkkrlab.nl/wiki/XBOX_360_RF_Module)

## Status

Untested against hardware — command values and clock-edge behaviour follow
the reference implementations and need bench verification. If a transaction
times out, `XboxRF::sendCommand()` returns `false` (module unpowered or bus
wiring/pull-ups wrong).

## Windows driver note

The PC recognises the module via the official Xbox 360 wireless receiver
driver with the `.inf` USB device IDs edited to match the salvaged module's
VID/PID. (Out of scope for this firmware.)
