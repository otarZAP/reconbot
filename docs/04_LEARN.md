# 🎓 How ReconBot Works — the learning guide

The fun part: understanding *why* every piece is there. Read a section before or
after building the matching stage. Each one is short on purpose.

---

## 1. Why a "pendulum" instead of a balancing Segway?

Both have two wheels on one axle. The difference is **where the weight sits**.

* **Self-balancer (Segway):** weight is *above* the axle. That's like balancing a
  broom on your hand — it always wants to fall, so a computer must read the tilt
  (with the MPU-6050) and constantly drive the wheels to catch it. This is a
  **closed-loop control** problem solved with a **PID controller**, and it's
  genuinely hard to tune — especially with cheap gear motors that have "backlash"
  (slop in the gears) and a delay before they respond.

* **Pendulum (what we built):** weight hangs *below* the axle. That's like a
  swing or a pendulum — gravity *pulls it upright on its own*. No sensor, no
  control loop, no tuning. It's **passively stable**. It still has that sleek
  two-wheeled drone look because the wheels and body shape are the same.

> Engineering lesson: the best way to solve a hard control problem is often to
> **redesign so you don't need control at all**. Moving the battery low turned a
> PID-tuning nightmare into a part that just works. Keep the self-balancer in
> your back pocket as a *future* project once the motors and code are familiar.

The MPU-6050 still earns a spot later: we can *read* the tilt and show it on the
HUD for telemetry — using the sensor without depending on it to stay upright.

---

## 2. How the motors are controlled — H-bridge + PWM

A DC motor has two terminals. Apply voltage one way → spins forward; flip the
voltage → spins backward. To flip voltage under software control we use an
**H-bridge** — four electronic switches in an "H" shape around the motor. The
**L298N** has two H-bridges (one per motor).

* The two **direction pins** (IN1/IN2 for the left motor) pick which way current
  flows → which way the wheel turns.
* The **enable pin** (ENA) turns the bridge on/off. We don't just turn it on — we
  flick it on-and-off *thousands of times a second*. The fraction of time it's
  on is the **duty cycle**. 100% on = full speed, 50% = roughly half speed. This
  trick is **PWM (Pulse-Width Modulation)**, and it's how almost everything
  controls power smoothly: motor speed, LED brightness, even class-D audio.

In the code, `ledcWrite(pin, duty)` sets the duty (0–255). The ESP32 has
dedicated **LEDC** hardware that generates the PWM so the main program is free
to do other things (like stream video).

> Detail worth knowing: the camera *also* uses an LEDC channel for its clock.
> That's why the code uses `ledcAttachChannel(...)` to put the motor PWM on
> *different* channels — two parts sharing one hardware resource would fight.

---

## 3. How video gets to your phone — MJPEG

Video sounds complicated, but ours is simple: the camera takes a **JPEG photo**,
sends it, takes another, sends it… ~20–30 times a second. Your eye blends them
into motion. That stream of JPEGs over one HTTP connection is called **MJPEG**
(Motion JPEG). The browser's `<img>` tag knows how to keep showing the newest
frame.

It's not as efficient as "real" video (H.264), but it's **dead simple and
low-latency**, which is perfect for driving a robot where you want to see *now*,
not a smooth-but-delayed picture.

---

## 4. How the controls reach the bot — a tiny web server

The ESP32 runs a small **web server**. Two jobs, on two ports:

* **Port 80** serves the HUD web page and listens for commands like
  `/control?joy=30,-80` (steer + drive) or `/control?cmd=S` (stop).
* **Port 81** serves the MJPEG video, so a slow video frame never blocks a
  control command.

As you move the on-screen joystick, the page sends `joy=x,y` (turn, forward).
Let go and it sends `cmd=S` (stop). That's the whole protocol — plain text in a
URL. You could literally type one into a browser to drive the bot.

> No app store, no install: the bot *is* a web server, and the browser already on
> the iPhone is the client. That's the same idea that runs the whole internet.

---

## 5. The failsafe — why the bot stops itself

What if you walk out of WiFi range, or close the app, while driving? The bot
would keep going. So the page sends a quiet "ping" every 0.4 s, and the firmware
remembers the time of the **last message**. In `loop()`, if more than
`FAILSAFE_MS` (0.7 s) passes with no message, it stops the motors.

> This is a **watchdog / deadman switch** — a core safety pattern in robotics,
> cars, and industrial machines: "if I stop hearing from the controller, assume
> the worst and go safe."

---

## 6. Why two separate power supplies?

Motors are *electrically loud*. When they start, they yank current and make the
voltage dip and spike. A sensitive brain (ESP32 + camera + WiFi radio) sharing
that same wire will **brown out and reset** — the single most common beginner
frustration with ESP32-CAM robots.

So we split power into two **rails**:

* **TPS63020 → 3.3 V → brain.** It's a **buck-boost** converter: it can step
  voltage *down or up*. Even when the battery sags under motor load, it *boosts*
  back up to a steady 3.3 V. The brain never notices the motors.
* **MT3608 → 5 V → motors.** All the noisy stuff lives here.

They only meet at the battery and at **common ground**. Clean brain, happy bot.

> Why not a 2-cell pack into the TPS63020 for more motor power? Because that part
> accepts **max 5.5 V in** — a single Li-ion cell only. Datasheets matter: one
> overlooked number can release the magic smoke. 🔌💨

---

## 7. The whole signal flow, one picture

```
   your thumb            iPhone (Safari)             ESP32-S3-CAM                 the world
  ───────────  touch  ─────────────────  WiFi  ──────────────────────────────  ──────────
   hold ▲      ─────►  send /control?cmd=F ───►  web server (port 80)
                                                    │
                                                    ├─► set motor pins + PWM ─►  L298N ─► wheels turn
   camera   ◄──────── <img> shows JPEGs ◄───── MJPEG stream (port 81) ◄── camera takes photos
                          (every 0.4s: ping ───► resets the failsafe timer)
```

That's the entire robot. Five small ideas — H-bridge, PWM, MJPEG, a web server,
and a watchdog — wired together. Once these click, you can build almost any
connected robot. 🟢
