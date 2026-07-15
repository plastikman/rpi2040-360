# xbox360-rf-rp2040

Firmware to drive a salvaged **Xbox 360 S (Slim / "Boron") Front Panel
Module** as a USB PC receiver for wireless Xbox 360 controllers, using a
**Waveshare RP2040-Zero** in place of the console SMC.

Inspired by the [Applied Carbon xboxrf mod](https://www.appliedcarbon.org/xboxrf.html)
(which targets the *phat* RF module), re-implemented for the RP2040 with the
[arduino-pico](https://github.com/earlephilhower/arduino-pico) core, and
adapted to the Slim FPM's control bus.

> **Slim, not phat.** Older writeups say Slim/S modules "aren't suitable"
> because the control bus differs. That's outdated — the Boron FPM exposes
> USB + 3.3V and its 2-wire bus was reverse-engineered in 2025
> ([drtrinity](https://drtrinity.uk/blog/2025/05/12/reversing-the-rf-board-1)).
> The bind/sync command is even identical to the phat's (`0000000100`).

## What it does

The FPM is itself a self-contained USB device — its USB lines go straight to
the PC. The RP2040 is **not** in the USB data path. It replaces the console
SMC toward the FPM and only:

- reads a **sync button**
- on power-on sends the mandatory Slim **"start module"** command + the
  ring-of-light **boot animation** (visible confirmation the bus works)
- triggers **controller binding** over the FPM's proprietary 2-wire bus

The **module** generates the clock (a **slow** ~hundreds-of-Hz clock on the
Slim, not the phat's ~250 kHz); the host pulls DATA low to start, sets each
bit ~1 ms into the clock-low phase, MSB-first. The Slim needs a mandatory
`0b0000010010` start command before LEDs/sync work, and its sync frame is
**11 bits** (phat's `0b0000000100` + a trailing `1`). Protocol matches the
working [ginokgx/xbox360slimRF](https://github.com/ginokgx/xbox360slimRF).

### Button behaviour

| Action | Result |
|--------|--------|
| Short press | Start controller binding (SYNC) |
| Long press (≥1.5 s) | Toggle the wireless radio off/on |

## Hardware

- Waveshare RP2040-Zero (USB-C, dual-core Cortex-M0+, 2MB flash)
- Salvaged Xbox 360 **S (Boron) Front Panel Module**
- 2× 10kΩ pull-up resistors (C_DATA, C_CLK)
- Momentary pushbutton
- USB cable (FPM → PC)

Wiring, full FPM connector pinout, and power rules: see
[`docs/wiring.md`](docs/wiring.md). Short version: the PC cable's 5V powers
the Zero via its `5V` pad, the Zero's `3V3` pad powers the FPM (pin 12), the
USB pair goes to FPM pins 5/6, and `GP3`/`GP4` are the C_DATA/C_CLK bus.

## Pin map

| RP2040-Zero GPIO | Function | FPM pin |
|------------------|----------|---------|
| `GP2` | Sync button (to GND, internal pull-up) | — |
| `GP3` | C_DATA (+10k pull-up) | 2 |
| `GP4` | C_CLK  (+10k pull-up) | 3 |

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

The Boron FPM two-wire bus and command frames are from community reverse
engineering. Command table lives in [`XboxRF.cpp`](XboxRF.cpp).

- [drtrinity — Reversing the RF board / SMC two-wire interface](https://drtrinity.uk/blog/2025/05/12/reversing-the-rf-board-1) (Slim/Boron, primary source)
- [agarmash.com writeup](https://agarmash.com/posts/xbox-360-controller-receiver/) (phat background)
- [tkkrlab wiki — XBOX 360 RF Module](https://tkkrlab.nl/wiki/XBOX_360_RF_Module) (phat)

## Status

Protocol reworked to match the working Slim implementation
[ginokgx/xbox360slimRF](https://github.com/ginokgx/xbox360slimRF) after a long
bring-up: the earlier phat-style assumptions (module clocks at 250 kHz, 10-bit
sync) were wrong for the Boron — the Slim clock is slow and needs the
`start module` init + an 11-bit sync frame. On boot the firmware plays the
ring-of-light **boot animation**, which is the quick visual check that the bus
is working. `sendData()` returns `false` on a clock-stall timeout.

Diagnostic firmwares (bus-monitor, bus-capture, bus-health, smc-clock-test)
live under [`tools/`](tools/).

## Windows driver note

The PC recognises the FPM via the official Xbox 360 wireless receiver driver
with the `.inf` USB device IDs edited to match the module's VID/PID.
(Out of scope for this firmware.)
