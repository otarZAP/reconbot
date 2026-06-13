# 📦 Bill of Materials

Most of these are common hobby-electronics parts, and several can be salvaged
from an Arduino starter/car kit. You shouldn't need anything exotic.

## Core build (required)

| # | Part | Qty | Role |
|---|------|----:|------|
| 1 | **ESP32-S3-CAM (FORIOT, OV3660, N16R8)** | 1 | Camera + brain + WiFi + control |
| 2 | **L298N dual H-bridge module** | 1 | Drives both wheels |
| 3 | **MG90S metal-gear micro servo** | 1 | Tilts the camera |
| 4 | **DC gear motors + wheels** | 2 | Drive — salvage from a toy/Arduino car kit |
| 5 | **18650 Li-ion cell** | 1 | Main battery |
| 6 | **18650 holder (single, wire leads)** | 1 | Holds the cell |
| 7 | **TP4056 Type-C charger w/ protection** | 1 | Charge + protect the cell |
| 8 | **TPS63020 / XL63020 buck-boost (3.3 V)** | 1 | Clean 3.3 V for the brain |
| 9 | **MT3608 boost converter** | 1 | 5 V for motors + servo |
| 10 | **Rocker switch (SPST)** | 1 | Master on/off |
| 11 | **Hookup wire / jumper wires** | — | Wiring |
| 12 | **Heatshrink, solder** | — | Tidy, durable joints |
| 13 | **3D-printed chassis + lid** | 1 set | Body + top cover (STLs included; wheels come from the car kit) |

> **Battery note:** use a good **18650** cell for the bot. Avoid tiny 1S LiPos
> (e.g. 450 mAh) — they're too small to drive the motors well. Only charge the
> cell on the TP4056, and never leave it charging unattended.

## Optional / later add-ons

| Part | Adds |
|------|------|
| **MPU-6050 IMU** | Live tilt/heading on the HUD (`USE_IMU 1`) |
| **DFPlayer Mini + 3 W speaker** | Startup chirp / "drone deployed" voice |
| **SSD1306 OLED 0.96"** | Tiny on-board status display |
| **LED** | Headlight on a spare GPIO |
| **NEO-6M GPS** | Outdoor position logging |
| **Spare ESP32-S3 devkit (e.g. XIAO ESP32-S3)** | Second brain / experiments |

## Tools

Soldering iron + solder + heatshrink · flush cutters · digital calipers ·
multimeter · breadboard + jumpers (for bench-testing power) ·
a 3D printer + PLA/PETG filament.

---

### A note on quantities
This is a forgiving first build. The ESP32-S3-CAM and the support parts are
cheap, so it's worth having a spare or two on hand — you can build more than one
drone and not stress about releasing a little magic smoke while you learn.
