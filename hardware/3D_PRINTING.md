# 🖨️ 3D Printing Guide

Two ready-to-print parts are included as STLs — no CAD needed to build:

* [`freecad/drone_chassis.stl`](../freecad/drone_chassis.stl) — the main body
* [`freecad/drone-lid.stl`](../freecad/drone-lid.stl) — the top cover

Drop them into your slicer and print. (The editable FreeCAD source isn't bundled
in this repo — export fresh STLs from your own model if you want to change the
geometry.)

> The **wheels are not printed** — they come from the salvaged car kit (see the
> [BOM](BOM.md)). The printed parts are just the body and its lid.

---

## What you'll print

| Part | File | Qty | Notes |
|------|------|----:|-------|
| **Chassis** | `drone_chassis.stl` | 1 | Main body — holds the motors, electronics, and 18650; hangs below the axle so the mass sits low. |
| **Lid** | `drone-lid.stl` | 1 | Closes the top of the chassis and protects the electronics. |

Total filament: small — a couple of hours, well under one spool.

---

## Step 1 — Slice & print (Bambu Studio / any slicer)

| Setting | Chassis & lid |
|---|---|
| Material | **PLA** (PETG if you want it tougher) |
| Layer height | 0.20 mm |
| Walls | 3 |
| Infill | 20–30 % |
| Supports | **Yes** for overhangs / the front camera window |
| Brim | optional (helps on small contact areas) |

> If your salvaged motors or boards are a different size than the model expects,
> the parts may not fit. In that case you'll need to tweak and re-export from
> your own CAD — start press-fit clearances around 0.25 mm (too tight → raise
> toward 0.35 mm; too loose → lower it).

## Step 2 — Test the fits before assembly

* Motors, the ESP32-S3-CAM, the L298N, and the 18650 should all sit in the
  **chassis** with the **battery lowest** (that low mass is what keeps the
  pendulum upright).
* The camera mounts to the servo horn so it can tilt — see the
  [build guide](../docs/01_BUILD_GUIDE.md).
* The **lid** should close over the top without crushing wires. Route cables
  first, then clip it on.
* **Car-kit wheels:** for more grip on smooth floors, wrap each tyre with a
  slice of silicone tape or a wide rubber band.

---

## Design intent (so you can restyle it)

The whole point of the shape is **low center of mass**: the chassis hangs
*below* the axle and the heavy 18650 lives at the bottom. That's what makes the
pendulum sit upright with no balancing code (see
[../docs/04_LEARN.md](../docs/04_LEARN.md) §1).

Want it to look more rugged/tactical? Things you can change in your own model:

* Add a faceted/angular shell around the body.
* Print a thin top "cage" so it reads as a rugged little recon unit.

Keep the **mass low and centered** and you can style the rest however you like.
