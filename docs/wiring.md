# Wiring — RP2040-Zero ↔ Xbox 360 RF module

## Principle

The RF module is a self-contained USB device. Its **USB lines go straight to
the PC** and never touch the RP2040. The RP2040 only drives the module's
2-wire control bus (LEDs + sync) and reads the physical sync button.

```
                 ┌──────────────────────────────────────┐
   USB to PC ────┤ D+  D-  VBUS(5V)  GND   (module USB)   │
                 │                                        │
                 │  Xbox 360 RF module                    │
                 │                                        │
                 │  3V3   GND   DATA   CLK   (control bus)│
                 └───┬─────┬─────┬──────┬─────────────────┘
                     │     │     │      │
        (module 3V3) │     │     │      │
                     │     │   +10k     +10k   (pull-ups to 3V3)
                     │     │     │      │
              ┌──────┴─────┴─────┴──────┴──────┐
   3V3 out ◄──┤ 3V3  GND   GP3    GP4          │
              │                    (DATA)(CLK) │
              │        RP2040-Zero             │
              │  GP2 ── sync button ── GND     │
              │  5V ◄── (from module VBUS/5V)  │
              └────────────────────────────────┘
```

## Connections

| RP2040-Zero | ↔ | Target | Notes |
|-------------|---|--------|-------|
| `5V` pad    | ← | module USB **VBUS (5V)** | powers the Zero; **use 5V as INPUT here** |
| `3V3` pad   | → | module **3.3V** rail | Zero's regulator powers the module logic |
| `GND`       | ↔ | module GND + button GND | common ground |
| `GP3`       | ↔ | module **DATA** | + 10kΩ pull-up to 3V3 |
| `GP4`       | ← | module **CLK** | + 10kΩ pull-up to 3V3 (module drives it) |
| `GP2`       | ← | sync **button** → GND | internal pull-up enabled in firmware |

Third recommended 10kΩ pull-up: on the button line if you don't trust the
internal pull-up, though `GP2` uses `INPUT_PULLUP` in firmware.

## USB / power rules

- **One 5V source at a time.** Power comes from the module's USB VBUS fed into
  the Zero's `5V` pad. Do **not** also plug the Zero's USB-C into a PC at the
  same time — the Zero (unlike a full Pico) may lack a backfeed protection
  diode, and the two 5V rails would fight.
- Use the Zero's **USB-C only for flashing firmware**, with the module's USB
  cable unplugged.
- The module's own USB cable is what the PC sees as the Xbox receiver.

## Before powering on

1. Confirm silkscreen: `3V3` is the regulator **output**, `5V` is VBUS
   in/out. Never feed 5V into the module's 3.3V rail.
2. Verify the module's current draw fits the Zero's onboard LDO headroom
   (a few hundred mA after the RP2040's ~50 mA). These modules are low-power,
   so this is almost always fine.
