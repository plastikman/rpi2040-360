# xbox360-rf-rp2040

> ## ⚠️ Status: DOES NOT WORK (with the Xbox 360 S "Boron" FPM)
>
> This project **does not currently work** with the Slim/S **Boron Front Panel
> Module** it targets. After an extensive bring-up, the Boron's 2-wire control
> bus **never produces a detectable clock** in response to the host, so no
> command (start / boot animation / sync) ever completes — every transaction
> times out. Both clock directions were tried (module-as-clock-master and
> host-as-clock-master); the module responds to neither.
>
> **What's confirmed working/correct:** the RP2040-Zero side, the wiring, the
> pinout (verified against the official BORON CONN `X819886-001` schematic),
> power/ground, and the build toolchain. The blocker is purely the module's
> undocumented control-bus behavior.
>
> **Why it's stuck:** there is **no published working reference for the Boron
> FPM control bus.** The one "working Slim" project
> ([ginokgx/xbox360slimRF](https://github.com/ginokgx/xbox360slimRF)) actually
> drives a **9-pin phat-style module** (pins 5/6/7), not the 13-pin Boron FPM.
>
> **What it needs next: logic-analyzer capture of a *live* Xbox 360 S console's
> SMC↔FPM traffic** to learn the real init/clock/framing. Without that ground
> truth, further progress is guesswork. (Alternatively, a **9-pin phat/RF02
> module** should work with this firmware, as USB enumerates on power alone.)
>
> Diagnostic firmwares used during bring-up live under [`tools/`](tools/).

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

## Status — does not work; needs logic analysis

See the notice at the top. Summary of the bring-up:

- Confirmed correct: RP2040-Zero, wiring, pinout (per `X819886-001`),
  power/ground, toolchain.
- The Boron FPM **never clocks** for us → all control-bus commands time out.
- Tried both clock directions; the module responds to neither.
- The current firmware follows the [ginokgx/xbox360slimRF](https://github.com/ginokgx/xbox360slimRF)
  protocol (module-clocked, slow clock, `start module` init `0b0000010010`,
  11-bit sync), but ginokgx's module is a **9-pin phat-style** module, not this
  13-pin Boron FPM — so it's not a guaranteed match, and it doesn't work here.
- USB never enumerated either (no device, no connect tone).

**Next step to unblock: a logic-analyzer capture of a live Xbox 360 S
console's SMC↔FPM control-bus traffic** — the real init/clock/framing. That's
the missing ground truth; everything else is guesswork without it.

Diagnostic firmwares (bus-monitor, bus-capture, bus-health, smc-clock-test)
live under [`tools/`](tools/).

## Windows driver note

The PC recognises the FPM via the official Xbox 360 wireless receiver driver
with the `.inf` USB device IDs edited to match the module's VID/PID.
(Out of scope for this firmware.)
