# xbox360-rf-rp2040

> ## ✅ Status: WORKING
>
> Drives the Xbox 360 S **Boron Front Panel Module** control bus (start module
> → ring-of-light boot animation → controller sync) from an RP2040-Zero.
>
> **Root cause of a very long bring-up: one mis-wired power pin.** 3.3V was
> tapped from the wrong pad, so the module was never actually powered — it
> never clocked, every command timed out, the bus lines sat at a mushy ~2.0V
> (back-feed through the signal pins' clamp diodes), and USB never enumerated.
> The moment 3.3V moved to the correct VCC pad, it worked with the existing
> firmware. **If nothing responds, check that VCC is really on the module's
> VCC net first — measure VCC→GND at the module, not just at your wire.**
>
> **Wire from the on-board pad row** (between the ribbon connector and the sync
> button), per drtrinity's Boron→Pico photo
> ([blog post](https://drtrinity.uk/blog/2025/05/12/reversing-the-rf-board-1)):
> **red = VCC (3.3V), black = GND, dark-blue = DATA → GP3, pink = CLK → GP4,
> cyan = USB D−, orange = USB D+.** Keep 1k pull-ups on DATA/CLK.
>
> Diagnostic firmwares from the bring-up live under [`tools/`](tools/).

---

Firmware to drive a salvaged **Xbox 360 S (Slim / "Boron") Front Panel
Module** as a USB PC receiver for wireless Xbox 360 controllers, using a
**Waveshare RP2040-Zero** in place of the console SMC.

Inspired by the [Applied Carbon xboxrf mod](https://www.appliedcarbon.org/xboxrf.html)
(which targets the *phat* RF module), re-implemented for the RP2040 with the
[arduino-pico](https://github.com/earlephilhower/arduino-pico) core.

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

## Status — working (controller pairs + registers input)

Confirmed end-to-end: the module **enumerates on USB**, a controller **pairs**
(RP2040 sync button → controller sync) and **registers input** in Windows
`joy.cpl`. On boot the firmware sends the Slim `start module` command then the
ring-of-light **boot animation** (visible confirmation); the sync command
completes over the bus.

**Windows driver:** the module reports `USB\VID_045E&PID_02A9`. Bind the stock
driver by force-picking it: Device Manager → the device → Update driver →
Browse → *Let me pick from a list* → Microsoft → **Xbox 360 Wireless Receiver
for Windows** (accept the incompatibility warning). No `.inf` edit needed.

**Known cosmetic gap:** the module's ring-of-light doesn't show the player-N
quadrant. The RP2040 only drives the control bus and isn't part of the USB-side
XInput player assignment, so it can't know which quadrant to light; there's no
confirmed Slim "set quadrant" command either. Functionally irrelevant.

Protocol follows [ginokgx/xbox360slimRF](https://github.com/ginokgx/xbox360slimRF):
module-clocked, **slow** clock (hundreds of Hz, not the phat's 250 kHz),
`start module` init `0b0000010010`, ring boot animation `0b0010000101`, 11-bit
sync `0b00000001001`, data set ~1 ms into the clock-low phase, MSB-first.

**The bug that cost the whole bring-up:** the 3.3V wire was on the wrong pad,
so the module was never powered — it never clocked, commands timed out, the
bus idled at a mushy ~2.0V, and USB wouldn't enumerate. Fixed by moving 3.3V
to the correct VCC pad. Lesson: **verify VCC→GND at the module itself.**

Diagnostic firmwares (bus-monitor, bus-capture, bus-health, smc-clock-test)
live under [`tools/`](tools/).

## Windows driver note

The PC recognises the FPM via the official Xbox 360 wireless receiver driver
with the `.inf` USB device IDs edited to match the module's VID/PID.
(Out of scope for this firmware.)
