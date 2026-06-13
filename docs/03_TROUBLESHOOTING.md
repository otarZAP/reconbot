# 🚑 ReconBot Troubleshooting

Work top-down: most problems are power or pins. Use the multimeter — guessing
wastes time.

---

## Uploading / board

**No COM port appears.**
* Use the **TTL** USB-C port (CH340C), **not** the OTG port.
* Try a different USB **cable** (many are charge-only with no data wires).

**Upload fails: "Failed to connect… No serial data received."** In order of
likelihood:
1. **Wrong port.** Use TTL, not OTG.
2. **Upload Speed too high.** Drop to **115200**.
3. **Bad cable.** Must be a data cable. Swap and test.
4. **Wrong button sequence.** Hold **BOOT** → tap **RST** → release **BOOT** →
   click Upload. Don't touch the buttons again.
5. **Battery rail on.** The power switch must be **OFF** while programming.
6. **Flaky BOOT button bypass:** short **GPIO0 to GND** with a wire while
   tapping RST, then release the wire.
7. **Dead BOOT button** (worked once, never again): the board is mechanically
   faulty — swap it. (You can't brick the chip; the ROM bootloader is permanent.)

**Compile error about `ledcAttach` / camera functions.**
* You're on an old core. Update **esp32 by Espressif** to **v3.0+** in Boards
  Manager. The v3.x LEDC API (`ledcAttach`) and these camera calls need it.

---

## Camera

**`CAMERA INIT FAILED 0x…` on the serial monitor.**
* **PSRAM** must be set to **OPI PSRAM** (Tools menu). This is almost always the
  cause — the camera won't init without OPI PSRAM.
* The pin map in `camera_pins.h` is taken from the FORIOT silkscreen pinout, so
  it usually isn't the pins. It's the PSRAM setting or a bad board.

**Video is upside-down or mirrored.**
* In `reconbot.ino`, in `cameraBegin()`, change `set_vflip` / `set_hmirror`
  (1 ↔ 0).

**HUD loads but video box stays black / "connecting".**
* The video lives on **port 81**. Some networks block it, but on the bot's own
  hotspot it's fine. Reload the page. The HUD auto-retries every second.
* Camera init may have failed — check the serial monitor (the bot still serves
  the page and drives even if the camera didn't start).

---

## Power (most "weird" problems live here)

**Camera/brain resets every time the motors move.**
* Classic shared-rail noise. The brain **must** be on the **TPS63020 3.3 V**
  rail, the motors on the **MT3608 5 V** rail — fed straight from the cell, not
  from each other. Recheck the power tree.
* Make sure **all grounds are tied together**.

**Nothing powers on.**
* Is the **switch** on? Battery charged? Measure TP4056 **OUT+** to **OUT−** —
  should be ~3.0–4.2 V.
* Measure the **two converter outputs** (3.3 V and 5.0 V). If a converter reads
  0 V, recheck its IN+ / IN− and that it's getting battery voltage.

**A module or chip gets hot fast.**
* **Switch off immediately.** Almost always reversed **+/−** on that module.
  Recheck polarity before powering again.

**TPS63020 died / outputs garbage.**
* It's a single-cell part (**max 5.5 V in**). If you fed it a 2-cell pack it's
  damaged. Use one of your spares and feed it from the single cell only.

---

## Motors

**D-pad does nothing.**
* ENA/ENB **jumpers removed**? With them on, sometimes; with our PWM they must be
  off — but also confirm the 6 signal wires match the table in the Wiring Guide.
* Is the L298N getting **+12V (VS)** *and* **+5V** *and* **GND**?
* Are the motors connected to OUT1/2 and OUT3/4?

**One wheel spins the wrong way.**
* Swap that motor's two output wires, **or** flip `INVERT_LEFT` /
  `INVERT_RIGHT` in `config.h` and re-upload.

**Motors weak / stall.**
* Raise the **MT3608** output toward 6 V (re-measure).
* Raise `DEFAULT_SPEED` in `config.h`.
* Cheap gear motors have a minimum speed — values below ~80 may just buzz; the
  HUD slider already starts at 80.

**Both wheels run only at full speed regardless of slider.**
* The **ENA/ENB jumpers are still on**. Remove them.

---

## Driving feel (pendulum chassis)

**Body swings hard / spins around the axle on launch.**
* Ease off the throttle (push the joystick less) or lower **`DEFAULT_SPEED`** in
  `config.h` — gentle throttle keeps the body from lurching.
* Move the **battery lower** in the chassis (lower center of mass = steadier).

**Won't stay upright at rest.**
* Center of mass must be **below** the axle. Add the battery at the very bottom
  of the pod; remove top-heavy items.

---

## Connection

**Phone can't find the `ReconBot` WiFi.**
* Give it ~5 s after power-on. Check the serial monitor printed `AP up`.
* Forget/re-join the network on the phone.

**Connected but `192.168.4.1` won't load.**
* Make sure you're on the **ReconBot** network (not your home WiFi). Ignore the
  "no internet" warning — that's normal.
* Type the full `http://192.168.4.1` (not https).
