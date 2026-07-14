# Wiring — RP2040-Zero ↔ Xbox 360 S (Boron) FPM

## Principle

The Front Panel Module is a self-contained USB device. Its **USB lines go
straight to the PC** and never touch the RP2040. The RP2040 only drives the
FPM's 2-wire control bus (power LED + controller binding) and reads a sync
button.

## Boron FPM connector pinout (X819886-001)

From the official BORON CONN schematic. We use only 6 of the 17 pins.

| Pin | Signal | Direction | Use here |
|----:|--------|-----------|----------|
| 1 | FP_TEMP_P | temp sensor | skip |
| **2** | **C_DATA** | control-bus data (BI) | → RP2040 `GP3` (+10k pull-up) |
| **3** | **C_CLK** | control-bus clock (FPM drives) | → RP2040 `GP4` (+10k pull-up) |
| 4 | SPARE1 | — | skip |
| **5** | **D−** | USB differential | → USB to PC |
| **6** | **D+** | USB differential | → USB to PC |
| 7 | FP_TEMP_N | temp sensor | skip |
| 8 | FP_PWR (PWRSW_N) | power button out | skip |
| **9** | **GND1** | ground | → GND |
| 10 | ODD_EJECT | eject button out | skip |
| **11** | **GND2** | ground | → GND |
| **12** | **V_3P3STBY** | 3.3V power | ← RP2040-Zero `3V3` pad |
| 13 | BINDSW_N | bind button out | skip (we sync over the bus) |
| 14 | ME1 | mount / GND | GND (optional) |
| 15 | ME2 | mount / GND | GND (optional) |
| 16 | ME3 | mount / GND | GND (optional) |
| 17 | ME4 | mount / GND | GND (optional) |

Note from schematic: `BORONFPMPORT_DX IS A USB DIFFERENTIAL PAIR` (pins 5/6).

## Connections

| RP2040-Zero | ↔ | FPM pin | Notes |
|-------------|---|---------|-------|
| `5V` pad    | ← | USB VBUS (5V, from the PC cable) | powers the Zero; 5V as **INPUT** |
| `3V3` pad   | → | **12** V_3P3STBY | Zero's regulator powers the FPM |
| `GND`       | ↔ | **9 / 11** (+ ME1–4) | common ground |
| `GP3`       | ↔ | **2** C_DATA | + 10kΩ pull-up to 3V3 |
| `GP4`       | ← | **3** C_CLK | + 10kΩ pull-up to 3V3 (FPM drives it) |
| `GP2`       | ← | sync button → GND | internal pull-up in firmware |

### USB routing

The PC's USB cable: **D+ → FPM pin 6, D− → FPM pin 5, GND → pin 9/11**, and
its **5V → the Zero's `5V` pad** (the FPM itself runs on 3.3V from the Zero,
not on USB 5V).

### Pull-ups and decoupling

- The console provides 100kΩ pull-ups on the bus lines *from the motherboard*.
  A salvaged FPM has none, so add **10kΩ pull-ups on C_DATA and C_CLK to 3V3**.
- The schematic puts a **100 µF bulk cap + 470 pF** on V_3P3STBY. Adding a
  ~10–100 µF cap across the module's 3.3V/GND near the connector is good
  practice for clean power.

## USB / power rules

- **One 5V source at a time.** Power the Zero from the PC cable's 5V (into the
  `5V` pad). Do **not** also plug the Zero's USB-C into a PC simultaneously —
  the Zero may lack backfeed protection and the rails would fight.
- Use the Zero's **USB-C only for flashing firmware**, module unplugged.

## Before powering on

1. Confirm silkscreen: `3V3` on the Zero is the regulator **output**; feed it
   to FPM pin 12 only. Never put 5V on pin 12.
2. Double-check pins 5/6 (D−/D+) and 2/3 (C_DATA/C_CLK) against your actual
   board with a multimeter — pin numbering/orientation on a bare connector is
   easy to mirror.
3. Verify FPM current draw fits the Zero's LDO headroom (low-power; fine).
