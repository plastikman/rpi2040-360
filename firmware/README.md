# Prebuilt firmware

`xbox360-rf-rp2040.uf2` — compiled binary, ready to flash without a toolchain.

- **Board:** Waveshare RP2040-Zero (`rp2040:rp2040:waveshare_rp2040_zero`)
- **Core:** arduino-pico (earlephilhower) `rp2040:rp2040@5.6.1`
- **Built from:** the sketch in the repo root

## Flash it

1. Hold **BOOT** on the RP2040-Zero, tap **RESET** (or plug in USB-C while
   holding BOOT). It mounts as a USB drive named `RPI-RP2`.
2. Drag `xbox360-rf-rp2040.uf2` onto that drive. The board reboots and runs.

## Rebuild

```bash
arduino-cli compile --fqbn rp2040:rp2040:waveshare_rp2040_zero .
```

> Untested on hardware — see the repo README for verification status.
