// ===========================================================================
//  config.h  —  ReconBot central configuration
// ---------------------------------------------------------------------------
//  This is the ONE file you edit to change WiFi, pins, and tuning.
//  Everything in the sketch reads from here, so you never have to hunt
//  through the code to change a pin number.
// ===========================================================================
#pragma once

// ---------------------------------------------------------------------------
//  1) WiFi mode
// ---------------------------------------------------------------------------
//  AP   = the bot makes its OWN WiFi hotspot. Your iPhone joins it directly.
//         Works anywhere, no home router needed. RECOMMENDED for a drone.
//  STA  = the bot joins your home WiFi. You browse to the IP it prints on the
//         serial monitor. Use this only if you want it on your home network.
//
//  Pick ONE by setting WIFI_MODE below.
//  NOTE: these are named RECON_* (not WIFI_MODE_AP/STA) on purpose — the ESP32
//  WiFi library already owns WIFI_MODE_AP / WIFI_MODE_STA as enum values, and
//  redefining them here breaks WiFi.mode(). Don't rename these back.
#define RECON_WIFI_AP    0
#define RECON_WIFI_STA   1

#define WIFI_MODE   RECON_WIFI_AP     // <-- change to RECON_WIFI_STA to join home WiFi

// --- Access-Point mode settings (the bot's own hotspot) -------------------
#define AP_SSID     "ReconBot"         // the WiFi name your phone will see
#define AP_PASS     "changeme123"      // 8+ characters. CHANGE THIS before you build!
#define AP_CHANNEL  1                 // 1, 6, or 11 are the cleanest channels
#define AP_HIDDEN   1                 // 1 = hidden SSID (won't show in WiFi lists),
                                      // 0 = visible. When hidden, add the network
                                      // MANUALLY on your phone (name = AP_SSID above).
// The bot is always reachable at  http://192.168.4.1  in AP mode.

// --- Station mode settings (join your home WiFi) --------------------------
#define STA_SSID    "YourHomeWiFi"
#define STA_PASS    "YourWiFiPassword"

// ---------------------------------------------------------------------------
//  2) Motor driver pins  (L298N)
// ---------------------------------------------------------------------------
//  Chosen for the FORIOT ESP32-S3-CAM (N16R8). They avoid the camera pins
//  (4-18, confirmed for this FORIOT board), the octal flash+PSRAM pins
//  (26-37), the USB pins (19,20), the serial/CH340 pins (43,44), and the
//  boot/strapping pins (0,3,45,46).
//
//  NOTE: GPIO 38/39/40 are the on-board microSD slot pins (1-bit SDIO). We
//  reuse them here because the drone doesn't use the SD card. >>> Leave the
//  microSD slot EMPTY <<< or the card and the motors will fight over the pins.
//
//  If any pin isn't broken out on a header pad on your board, pick another
//  free pin (good spares: 21, 47, 48, 1, 2) and change it HERE only. Check
//  your board's silkscreen / pinout image to see which pads are broken out.
//
//  Motor A = LEFT wheel, Motor B = RIGHT wheel  (swap later if needed).
#define MOTOR_A_IN1   42     // direction pin 1 (Motor A / left)
#define MOTOR_A_IN2   41     // direction pin 2 (Motor A / left)
#define MOTOR_A_PWM   40     // ENA - speed (PWM) for Motor A / left  (SD D0 pad)
#define MOTOR_B_IN3   39     // direction pin 1 (Motor B / right)     (SD CLK pad)
#define MOTOR_B_IN4   38     // direction pin 2 (Motor B / right)     (SD CMD pad)
#define MOTOR_B_PWM   47     // ENB - speed (PWM) for Motor B / right

// If the bot drives BACKWARD when you press forward, flip these to true.
#define INVERT_LEFT   false
#define INVERT_RIGHT  false

// ---------------------------------------------------------------------------
//  3) Camera tilt servo  (MG90S)
// ---------------------------------------------------------------------------
#define SERVO_PIN     21
#define SERVO_MIN_DEG 30     // lowest tilt the mechanics allow (don't hit body)
#define SERVO_MAX_DEG 150    // highest tilt
#define SERVO_START_DEG 90   // where it points on power-up (level)

// ---------------------------------------------------------------------------
//  4) Optional: MPU-6050 IMU (tilt read-out shown on the web HUD)
// ---------------------------------------------------------------------------
//  Leave USE_IMU 0 for the first build. Turn on later as a fun add-on.
#define USE_IMU       0
#define IMU_SDA       1
#define IMU_SCL       2

// ---------------------------------------------------------------------------
//  5) Driving feel / safety
// ---------------------------------------------------------------------------
#define PWM_FREQ      1000   // motor PWM frequency in Hz (1 kHz is quiet+smooth)
#define PWM_RES_BITS  8      // 8-bit -> speed values 0..255
#define DEFAULT_SPEED 200    // TOP speed (0-255): full joystick push = this duty.
                             // (There's no speed slider anymore — the stick IS
                             // the throttle. Lower this if it's still too quick.)
#define TURN_SPEED_SCALE 50  // steering sensitivity: how much the joystick's
                             // left/right push affects the wheels (%). Lower =
                             // gentler, easier-to-aim turns; higher = sharper.

// JOYSTICK FEEL: shape the raw stick so small pushes give small movement.
//  JOY_EXPO 0  = linear (twitchy);  100 = very soft centre, fine control.
//  JOY_DEADZONE ignores tiny pushes near centre so the bot doesn't creep/jitter.
#define JOY_EXPO      55     // 0..100 — softness of the centre
#define JOY_DEADZONE  6      // 0..100 — dead spot around centre

// SOFT-START: the motors ease toward their target speed instead of slamming to
// it, so starts, stops, and turns are smooth. This is the max speed CHANGE per
// 20 ms loop tick (0..255). Lower = smoother/slower response; higher = snappier.
// 20 -> roughly a quarter-second from a standstill to full speed.
#define ACCEL_STEP    20

// FAILSAFE: if the phone stops sending commands for this many milliseconds
// (app closed, walked out of range), the bot stops itself. Safety first.
#define FAILSAFE_MS   700

// ---------------------------------------------------------------------------
//  6) Camera resolution
// ---------------------------------------------------------------------------
//  Normal driving always stays at DRIVE_FRAMESIZE — smooth + responsive. The
//  HD button in the app bumps the feed UP toward HIRES_CEILING: the firmware
//  picks the highest size the current WiFi signal (RSSI) can comfortably push,
//  capped here. Untick HD and it drops straight back to DRIVE_FRAMESIZE.
//
//  HOW IT WORKS: the camera boots at HIRES_CEILING so its PSRAM buffers are big
//  enough, then runs DOWN to DRIVE_FRAMESIZE. (Going small->big at runtime is
//  what glitches the OV2640, so we only ever shrink from the max.)
//  Needs PSRAM (you have it). If HD feels too heavy, drop UXGA -> SVGA.
#define DRIVE_FRAMESIZE  FRAMESIZE_VGA    // 640x480   — the native / driving feed
#define HIRES_CEILING    FRAMESIZE_UXGA   // 1600x1200 — most the HD button can reach

// ---------------------------------------------------------------------------
//  7) Optional headlight LED
// ---------------------------------------------------------------------------
//  A little front light for dark rooms, toggled from the web LIGHT button.
//  You don't have the LED wired yet, so this is OFF by default — the button
//  still appears in the app but won't do anything until you flip HAS_HEADLIGHT
//  to 1. When you add it: LED + a ~220 ohm resistor from LED_PIN to GND.
#define HAS_HEADLIGHT    0    // 0 = not wired yet; set to 1 once the LED is in
#define LED_PIN          48   // a free GPIO (good spares on this board: 48, 1, 2)
#define LED_ACTIVE_HIGH  1    // 1 = pin HIGH lights the LED; 0 = wired inverted
