# 🔌 ReconBot Wiring Guide

Every wire, every pin. Wire with the **battery removed and the power switch
OFF**. Work slowly and check each connection against the tables before you
apply power.

> **Camera pins + flash settings are confirmed** from the FORIOT
> ESP32-S3-CAM's silkscreen pinout — no guessing there. The **motor GPIOs**
> below are a sound default chosen to dodge the camera, flash/PSRAM, USB,
> serial, and boot pins. If one isn't broken out on a header pad on your board,
> pick another free pin (good spares: 21, 48, 1, 2) and change it in
> [`config.h`](../firmware/reconbot/config.h) — that's the only place to edit.
> Check your board's silkscreen / pinout diagram to confirm which pads are
> broken out.

---

## 1. The power tree (build this first, test it before anything else)

One 18650 cell. The brain and the muscle live on **separate rails** on purpose:
motors create electrical noise and voltage dips, and if the camera shared that
rail it would constantly reset. The TPS63020 also *boosts* — so even when the
cell sags under motor load, the brain still gets a steady 3.3 V.

```
                       ┌─────────────────────────────────────────────┐
                       │                                             │
  [18650 +]──► TP4056 OUT+ ──► [SWITCH] ──┬──► TPS63020 IN+ ──► OUT 3.3V ──► ESP32-S3-CAM  3V3
  [18650 -]──► TP4056 OUT- ───────────────┤    TPS63020 IN- ──► OUT GND  ──► ESP32-S3-CAM  GND
                                          │
                                          ├──► MT3608 IN+ ──► OUT 5V ──┬──► L298N  +12V (VS)
                                          │    MT3608 IN- ──► OUT GND  └──► L298N  +5V  (logic, jumper OFF)
                                          │
                                          └────────────────── COMMON GROUND ──────────────────┐
                                                                                               │
   ALL grounds tie together:  18650- / TP4056 OUT- / TPS63020 GND / MT3608 GND /              │
   L298N GND / ESP32 GND ──────────────────────────────────────────────────────────────────────┘
```

**Set the converter voltages with a multimeter BEFORE connecting any board:**

* **TP4056** — battery goes on **B+ / B−**; the bot draws from **OUT+ / OUT−**.
  Charge any time via the TP4056's USB-C. (Protected version — good.)
* **TPS63020 (XL63020)** — fixed **3.3 V** out. Confirm it reads ~3.3 V.
* **MT3608** — turn its trim-pot until the output reads **5.0 V**. Do this with
  nothing connected to the output yet. (Want peppier motors later? 6 V is fine.)

> ⚠️ **Never feed more than 5.5 V into the TPS63020** — it's a single-cell part.
> That's why we power the brain straight from the one cell, not from a 2-cell pack.

> ⚠️ **Power switch OFF before connecting USB** to the ESP32 for programming.
> Otherwise USB-5V and the 3.3 V rail both try to power the board and fight.

---

## 2. ESP32-S3-CAM → L298N (motor driver)

The ESP32 sends 6 logic signals to the L298N: 2 direction + 1 speed (PWM) per motor.

| ESP32-S3 pin | → L298N pin | Meaning |
|---|---|---|
| GPIO **42** | IN1 | Left motor direction A |
| GPIO **41** | IN2 | Left motor direction B |
| GPIO **40** | ENA | Left motor speed (PWM) |
| GPIO **39** | IN3 | Right motor direction A |
| GPIO **38** | IN4 | Right motor direction B |
| GPIO **47** | ENB | Right motor speed (PWM) |
| **GND** | GND | shared ground (also in the power tree) |

> **ENA/ENB jumpers:** the L298N ships with little jumpers on ENA/ENB that force
> full speed. **Remove both jumpers** so our PWM speed control works.

> **The "+5V logic" jumper:** there's a jumper near the +5V terminal that
> enables the on-board 5 V regulator. Since we feed 5 V in from the MT3608,
> **remove that jumper** and feed the **+5V** terminal from the 5 V rail instead.

> **microSD note:** GPIO 38/39/40 are the FORIOT board's microSD slot pins
> (confirmed from the FORIOT pinout). We borrow them for motors, so
> **leave the microSD slot empty** for this build.

### L298N → motors

| L298N output | → wire | Motor |
|---|---|---|
| OUT1 / OUT2 | the two leads | **Left** gear motor |
| OUT3 / OUT4 | the two leads | **Right** gear motor |

Polarity doesn't matter yet — if a wheel spins the wrong way during testing,
either swap that motor's two wires **or** flip `INVERT_LEFT` / `INVERT_RIGHT`
in `config.h`. No re-soldering needed.

---

## 3. (Optional, later) MPU-6050 IMU

Skip this for the first build. When you want a live tilt/heading read-out on the
HUD, set `USE_IMU 1` in `config.h` and wire:

| MPU-6050 | → ESP32-S3 |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SDA | GPIO **1** |
| SCL | GPIO **2** |

(This is a *second* I²C bus, separate from the camera's internal one.)

---

## 4. Pre-power checklist

Before the very first switch-on:

- [ ] **+ and −** correct at every module (look twice).
- [ ] TPS63020 output reads **3.3 V**, MT3608 output reads **5.0 V** (measured, loaded later).
- [ ] **All grounds connected together** (brain, motors, converters, battery).
- [ ] ENA/ENB jumpers **removed**; L298N +5V regulator jumper **removed**.
- [ ] microSD slot **empty**.
- [ ] Wheels **off the ground**.
- [ ] USB **unplugged** (or switch off) so power sources don't fight.

First switch-on: the ESP32's small LED should light, and within ~5 s the
`ReconBot` WiFi network should appear on your phone. If anything smells hot or a
chip gets warm fast — **switch off immediately** and recheck polarity.

---

## Pin reference (one-page summary)

```
ESP32-S3-CAM
 ├─ 3V3  ◄── TPS63020 3.3V        (brain power)
 ├─ GND  ──  common ground
 ├─ G42 ──► L298N IN1   (L dir)
 ├─ G41 ──► L298N IN2   (L dir)
 ├─ G40 ──► L298N ENA   (L speed/PWM)
 ├─ G39 ──► L298N IN3   (R dir)
 ├─ G38 ──► L298N IN4   (R dir)
 └─ G47 ──► L298N ENB   (R speed/PWM)

L298N
 ├─ +12V (VS) ◄── 5V rail (MT3608)
 ├─ +5V       ◄── 5V rail (MT3608)   [regulator jumper OFF]
 ├─ GND       ──  common ground
 ├─ OUT1/OUT2 ──► Left motor
 └─ OUT3/OUT4 ──► Right motor
```

Next: assemble it all in [01_BUILD_GUIDE.md](01_BUILD_GUIDE.md).
