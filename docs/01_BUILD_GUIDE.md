# 🛠️ ReconBot Build Guide

A staged build. Each stage **ends in a test** — don't move on until the test
passes. This is the secret to "smooth sailing": you never debug ten things at
once, only the one thing you just added.

**Time:** ~a weekend, split into sessions. Great pace for building with a teen.

**Tools you have:** soldering iron + heatshrink, flush cutters, calipers,
multimeter (from the ELEGOO kit / power supply), the P1S printer.

---

## Stage 0 — Set up the computer (30 min)

1. Install the **Arduino IDE 2.x** (free, arduino.cc).
2. Add ESP32 support: **File ▸ Preferences ▸ Additional Boards Manager URLs**,
   paste:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. **Tools ▸ Board ▸ Boards Manager**, search **esp32**, install **"esp32" by
   Espressif Systems** (v3.0 or newer).
4. Plug the ESP32-S3-CAM into the computer using the **USB-C port labelled
   "TTL"** (the CH340C port). The **OTG** port is *not* for flashing. A COM port
   should appear under **Tools ▸ Port**.

✅ **Test:** the board appears as a COM port. (No code yet.)

> If you've flashed an ESP32-S3-CAM before, the BOOT/RST dance below will be
> familiar — same procedure applies here.

---

## Stage 1 — Flash the ReconBot firmware (15 min)

The camera pin map is **already confirmed** (taken from the FORIOT board's
silkscreen pinout and verified on hardware), so we skip straight to the firmware.

1. Open [`firmware/reconbot/reconbot.ino`](../firmware/reconbot/reconbot.ino).
   The folder also has `config.h`, `camera_pins.h`, `web_index.h` — keep all
   four together; the IDE opens them as tabs.
2. In **`config.h`**, set your hotspot password (`AP_PASS`) to something you'll
   remember. Leave everything else at defaults for now.
3. Set **Tools** to these proven settings:
   * Board: **ESP32S3 Dev Module**
   * PSRAM: **OPI PSRAM**  ← mandatory; camera won't init without it
   * Flash Size: **16MB (128Mb)**
   * Partition Scheme: **16M Flash (3MB APP/9.9MB FATFS)** (or **Huge APP**)
   * USB CDC On Boot: **Disabled**
   * Upload Speed: **115200**  (lower = more reliable on the CH340)
4. **Enter download mode:** hold **BOOT**, tap **RST**,
   release **BOOT**, then click **Upload** immediately.
5. Open **Tools ▸ Serial Monitor** at **115200**. You should see:
   `AP up. Join WiFi 'ReconBot', then open http://192.168.4.1`.
6. On your iPhone: **Settings ▸ WiFi ▸ ReconBot**, enter the password. Then open
   **Safari ▸ `http://192.168.4.1`**.

✅ **Test:** the green tactical HUD loads on your phone with live video. (No
motors yet — the D-pad does nothing because the L298N isn't wired. That's fine.)

> Upload won't connect? It's almost always the wrong USB-C port (use TTL), the
> upload speed, or the BOOT/RST timing — see
> [03_TROUBLESHOOTING.md](03_TROUBLESHOOTING.md). iPhone may warn "no internet"
> on the ReconBot network — expected; stay connected anyway. WiFi is 2.4 GHz.

> **Optional confidence check:** if you'd rather prove the camera before our
> code, flash **File ▸ Examples ▸ ESP32 ▸ Camera ▸ CameraWebServer** (model
> `CAMERA_MODEL_ESP32S3_EYE`) first — but since the pin map already matches your
> working cameras, you can skip this.

---

## Stage 2 — Print the parts (print while you build)

Start these prints now; they run in the background. Settings + the sleek,
compact pendulum design are in [hardware/3D_PRINTING.md](../hardware/3D_PRINTING.md).

You'll print just two parts — the included STLs: the **chassis** (the main body;
it holds the ESP32 + L298N + battery, mounts the motors, and *hangs below the
axle*) and a **lid** that closes the top. The **wheels come from the car kit**,
not the printer.

✅ **Test:** the chassis comes off the plate clean; the ESP32 and L298N sit in
their slots and the 18650 drops into the bottom.

---

## Stage 3 — Build & test the power tree (45 min)

Follow [02_WIRING_GUIDE.md](02_WIRING_GUIDE.md) §1. Build power **on the
breadboard first**, not soldered to the bot.

1. 18650 → TP4056 (**B+/B−**). Bot power comes from TP4056 **OUT+/OUT−**.
2. Add the **switch** in the OUT+ line.
3. Wire the **TPS63020** input to the switched rail. Switch on. Measure its
   output: **3.3 V**. Switch off.
4. Wire the **MT3608** input to the switched rail. Switch on. Adjust its pot to
   **5.0 V** on the output. Switch off.

✅ **Test:** multimeter reads 3.3 V and 5.0 V on the two rails, and nothing gets
warm. Don't connect any boards until both rails are confirmed.

---

## Stage 4 — Wire & test the motors (45 min)

1. **Switch off / USB unplugged.** Wire ESP32 → L298N per
   [02_WIRING_GUIDE.md](02_WIRING_GUIDE.md) §2 (the 6 signal pins + GND).
2. Remove the **ENA/ENB jumpers** and the **+5V regulator jumper** on the L298N.
3. Feed L298N **+12V (VS)** and **+5V** from the 5 V rail; connect **GND**.
4. Connect the two motors to OUT1/2 and OUT3/4. **Keep wheels off the ground.**
5. Power the brain from the 3.3 V rail (3V3 + GND). Switch on.
6. Connect the phone, open the HUD, and press the D-pad.

✅ **Test:** Forward = both wheels spin the same way (the way that would move the
bot forward). Use Left/Right and the **speed slider**.

* A wheel spins backward? Swap that motor's two output wires, **or** set
  `INVERT_LEFT`/`INVERT_RIGHT` in `config.h` and re-upload.
* Camera resets when motors start? Your rails aren't separate — recheck that the
  brain is on the **TPS63020** rail, not the motor rail. (See Troubleshooting.)

> Notice the **failsafe**: let go of the D-pad and the wheels stop. Close the
> app and within ~0.7 s the bot stops itself.

---

## Stage 5 — Wire & test the servo (20 min)

1. Switch off. Wire the **MG90S**: orange→GPIO 21, red→5 V rail, brown→GND
   ([02_WIRING_GUIDE.md](02_WIRING_GUIDE.md) §3).
2. Switch on, open the HUD, move the **CAM TILT** slider.

✅ **Test:** the servo horn tilts smoothly across its range and holds position.

> If it buzzes at the ends, narrow `SERVO_MIN_DEG`/`SERVO_MAX_DEG` in `config.h`
> so it never pushes against a mechanical stop.

---

## Stage 6 — Mechanical assembly (the pendulum chassis) (1–2 hr)

This is what gives it its distinctive look. The key idea: **the two motors mount on a
single axle line, and the chassis hangs below them.** Low weight = naturally
upright.

1. **Mount the motors** in the chassis so both output shafts are **coaxial**
   (pointing left and right along one straight line).
2. **Press the car-kit wheels** onto the shafts.
3. The **chassis hangs below** the shaft line so its mass sits *below* the axle.
   Put the **heaviest item (the 18650) at the very bottom** — the lower the
   battery, the more stable and upright it sits.
4. **Mount the servo + camera** on the front of the chassis: the servo body
   fixes to the chassis, the camera fixes to the servo horn so it tilts up/down.
5. Route wires neatly, secure with a little tape or zip-ties, leave slack at the
   servo so it can move, then **clip the lid on** top.

✅ **Test (the magic moment):** set it on the floor, hands off. It should sit
upright on its two wheels, body hanging below the axle. A gentle
nudge and it rocks back upright.

> If it won't stay upright: move the battery **lower**, and make sure the body's
> center of mass is **below** the axle, not level with it. If it spins around the
> axle hard when you hit Forward, lower the **speed slider** and/or raise
> `TURN_SPEED_SCALE` smoothing — gentle starts keep the body from swinging.

---

## Stage 7 — First drive & tuning (ongoing fun)

* **Speed slider** low to start. The pendulum body will bob when you accelerate —
  that's authentic and expected. Smooth, gentle throttle looks best.
* **Camera tilt** up to "scan" a room like the drone.
* Tune the *feel* in `config.h`: `DEFAULT_SPEED`, `TURN_SPEED_SCALE`,
  servo limits. Re-upload to try changes.

### Ideas to grow it (great next lessons)

* Turn on the **MPU-6050** (`USE_IMU 1`) and show live tilt on the HUD.
* Add a **headlight LED** on a spare GPIO with a button on the HUD.
* Add sound with a **DFPlayer Mini + speaker** (you have both) — a startup chirp
  or "drone deployed" voice line.
* Try **`WIFI_MODE_STA`** so it joins home WiFi and you drive from any room.

See the physics and the "why" behind every choice in
[04_LEARN.md](04_LEARN.md).
