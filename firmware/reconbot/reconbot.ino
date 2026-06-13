// ===========================================================================
//  ReconBot  —  ESP32-S3-CAM recon drone firmware
// ---------------------------------------------------------------------------
//  ONE ESP32-S3-CAM does everything:
//    * streams live video to your phone (MJPEG, port 81)
//    * serves the control web app (port 80)
//    * drives two motors through an L298N (virtual-joystick steering)
//    * smooth soft-start ramping so starts/stops/turns aren't jerky
//    * drives at 640x480; HD button bumps resolution as the signal allows
//    * stops itself if the phone disconnects (failsafe)
//
//  Edit pins / WiFi / tuning in  config.h  — not here.
//  Camera pins live in  camera_pins.h.  Web app lives in  web_index.h.
//
//  Board settings in Arduino IDE (Tools menu) — these are the SAME settings
//  proven on the FORIOT ESP32-S3-CAM (N16R8):
//    Board:            "ESP32S3 Dev Module"
//    PSRAM:            "OPI PSRAM"        <-- MANDATORY; camera won't init without it
//    Flash Size:       "16MB (128Mb)"
//    Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)"  (or "Huge APP")
//    USB CDC On Boot:  "Disabled"         <-- we use UART0 over the CH340 "TTL" port
//    Upload Speed:     "115200"           <-- lower = more reliable on the CH340
//  Plug into the USB-C port labelled TTL (CH340C), NOT the OTG port.
//  To upload: hold BOOT, tap RST, release BOOT, then click Upload.
//  Needs the ESP32 board package v3.x (Boards Manager -> "esp32" by Espressif).
//  No extra libraries to install — everything used here ships with the core.
// ===========================================================================
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_wifi.h"          // for the connected-phone signal strength (RSSI)
#include "esp_http_server.h"
#include "esp_timer.h"

#include "config.h"
#include "camera_pins.h"
#include "web_index.h"

#if USE_IMU
  #include <Wire.h>
#endif

httpd_handle_t control_httpd = NULL;   // port 80: web app + commands
httpd_handle_t stream_httpd  = NULL;   // port 81: video

volatile uint32_t g_lastCmdMs = 0;     // when we last heard from the phone
int  g_speed   = DEFAULT_SPEED;        // 0..255 (top speed; joystick scales within)
int  g_trim    = 0;                    // -100..100 steering trim (drive-straight tuning)
bool g_moving  = false;                // are we commanding any wheel movement?

// Virtual-joystick input, normalised to -100..100 (x = turn, y = forward).
int  g_jx = 0, g_jy = 0;

// Soft-start ramping: we move the CURRENT wheel duties toward the TARGET duties
// a little each loop tick (signed, -255..255; sign = direction).
int  g_tgtL = 0, g_tgtR = 0;
int  g_curL = 0, g_curR = 0;

volatile bool g_dancing = false;       // true while the DANCE routine owns the motors

// Camera resolution state (see config.h section 6)
bool        g_psram    = false;              // true if PSRAM present -> hi-res allowed
bool        g_hires    = false;              // HD button state
framesize_t g_curFrame = DRIVE_FRAMESIZE;    // resolution the sensor is at right now

// ---------------------------------------------------------------------------
//  MOTORS
// ---------------------------------------------------------------------------
void motorsBegin() {
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN3, OUTPUT);
  pinMode(MOTOR_B_IN4, OUTPUT);
  // Hardware PWM on the enable pins (auto-allocated from the bottom timers;
  // the camera sits on the top timer/channel so there's no clash).
  ledcAttach(MOTOR_A_PWM, PWM_FREQ, PWM_RES_BITS);
  ledcAttach(MOTOR_B_PWM, PWM_FREQ, PWM_RES_BITS);
  ledcWrite(MOTOR_A_PWM, 0);
  ledcWrite(MOTOR_B_PWM, 0);
}

// dir: +1 forward, -1 backward, 0 coast.  duty: 0..255
void setLeft(int dir, int duty) {
  if (INVERT_LEFT) dir = -dir;
  digitalWrite(MOTOR_A_IN1, dir > 0);
  digitalWrite(MOTOR_A_IN2, dir < 0);
  ledcWrite(MOTOR_A_PWM, dir == 0 ? 0 : duty);
}
void setRight(int dir, int duty) {
  if (INVERT_RIGHT) dir = -dir;
  digitalWrite(MOTOR_B_IN3, dir > 0);
  digitalWrite(MOTOR_B_IN4, dir < 0);
  ledcWrite(MOTOR_B_PWM, dir == 0 ? 0 : duty);
}

// Push a SIGNED duty (-255..255) to one wheel: sign picks direction, |value| the speed.
void applyWheel(bool left, int v) {
  int dir  = (v > 0) ? 1 : (v < 0 ? -1 : 0);
  int duty = abs(v);
  if (left) setLeft(dir, duty); else setRight(dir, duty);
}

// Hard stop + full reset so a ramp can't sneak the wheels back on.
void motorsStop() {
  g_jx = g_jy = 0;
  g_tgtL = g_tgtR = 0;
  g_curL = g_curR = 0;
  g_moving = false;
  applyWheel(true, 0);
  applyWheel(false, 0);
}

// Set where the wheels SHOULD end up; the loop ramps them there smoothly.
void setTargets(int left, int right) {
  g_tgtL = constrain(left,  -255, 255);
  g_tgtR = constrain(right, -255, 255);
  g_moving = (g_tgtL != 0 || g_tgtR != 0);
}

// Arcade mixing: turn the joystick (forward y + turn x) into left/right wheel
// targets, scaled by g_speed (DEFAULT_SPEED), with steering trim folded in.
void mixDrive() {
  int turn  = (g_jx * TURN_SPEED_SCALE) / 100;     // gentle, aim-able steering
  int left  = ((g_jy + turn) * g_speed) / 100;
  int right = ((g_jy - turn) * g_speed) / 100;
  // Trim eases ONE wheel a touch so the bot tracks straight (same idea as before).
  if (g_trim > 0)      right = (right * (100 - g_trim)) / 100;
  else if (g_trim < 0) left  = (left  * (100 + g_trim)) / 100;
  setTargets(left, right);
}

// Shape a raw axis (-100..100): dead-zone near centre, then an expo curve so a
// small push gives a small move while full deflection still reaches full speed.
//   out = (1-e)*x  +  e*x^3   (classic RC expo), all in integer math.
int shapeAxis(int v) {
  v = constrain(v, -100, 100);
  if (v > -JOY_DEADZONE && v < JOY_DEADZONE) return 0;
  long lin = ((long)(100 - JOY_EXPO) * v) / 100;
  long cub = ((long)JOY_EXPO * v * v * v) / 1000000L;   // x^3 scaled back to ±100
  return (int)(lin + cub);
}

void applyJoystick(int x, int y) {
  g_jx = shapeAxis(x);
  g_jy = shapeAxis(y);
  mixDrive();
}

// Move a value toward a goal by at most 'step' (the heart of the soft-start).
int stepToward(int cur, int goal, int step) {
  if (cur < goal) return min(cur + step, goal);
  if (cur > goal) return max(cur - step, goal);
  return cur;
}

// Called every loop tick: ease the wheels toward their targets and drive them.
void rampMotors() {
  g_curL = stepToward(g_curL, g_tgtL, ACCEL_STEP);
  g_curR = stepToward(g_curR, g_tgtR, ACCEL_STEP);
  applyWheel(true,  g_curL);
  applyWheel(false, g_curR);
}

// ---------------------------------------------------------------------------
//  DANCE  —  a quick "just for fun" freestyle. Bypasses ramping for snappy
//  moves and takes over the motors for ~4 s (loop() backs off while g_dancing).
// ---------------------------------------------------------------------------
void danceRoutine() {
  g_dancing = true;
  const int sp = 190;
  // 1) shimmy: snap left/right a few times
  for (int i = 0; i < 4; i++) {
    setLeft( 1, sp); setRight(-1, sp); delay(170);
    setLeft(-1, sp); setRight( 1, sp); delay(170);
  }
  // 2) victory spins
  setLeft( 1, sp); setRight(-1, sp); delay(750);
  setLeft(-1, sp); setRight( 1, sp); delay(750);
  // 3) bounce forward/back
  for (int i = 0; i < 3; i++) {
    setLeft( 1, sp); setRight( 1, sp); delay(150);
    setLeft(-1, sp); setRight(-1, sp); delay(150);
  }
  motorsStop();
  g_lastCmdMs = millis();        // don't let the failsafe fire right after
  g_dancing = false;
}

// ---------------------------------------------------------------------------
//  HEADLIGHT LED  (optional — only does anything when HAS_HEADLIGHT == 1)
// ---------------------------------------------------------------------------
void lightBegin() {
#if HAS_HEADLIGHT
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_ACTIVE_HIGH ? LOW : HIGH);   // start OFF
#endif
}
void setLight(bool on) {
#if HAS_HEADLIGHT
  bool level = LED_ACTIVE_HIGH ? on : !on;
  digitalWrite(LED_PIN, level ? HIGH : LOW);
#endif
}

// ---------------------------------------------------------------------------
//  CAMERA
// ---------------------------------------------------------------------------
bool cameraBegin() {
  camera_config_t c = {};                      // zero everything first (important!)
  // The camera generates its clock (XCLK) with an LEDC unit. A plain camera
  // build puts it on channel0/timer0, but that has no other PWM. The drone
  // adds motor PWM, so we move the camera to the HIGHEST timer/channel (3 / 7).
  // The motor PWM below uses the Arduino LEDC auto-allocator, which hands out
  // timers/channels from the BOTTOM (0,1,2...). Keeping the camera up high means
  // the two never collide — which would otherwise break the video or the motor
  // speed. (XCLK works on any LEDC unit.)
  c.ledc_channel = LEDC_CHANNEL_7;
  c.ledc_timer   = LEDC_TIMER_3;
  c.pin_d0=Y2_GPIO_NUM; c.pin_d1=Y3_GPIO_NUM; c.pin_d2=Y4_GPIO_NUM; c.pin_d3=Y5_GPIO_NUM;
  c.pin_d4=Y6_GPIO_NUM; c.pin_d5=Y7_GPIO_NUM; c.pin_d6=Y8_GPIO_NUM; c.pin_d7=Y9_GPIO_NUM;
  c.pin_xclk=XCLK_GPIO_NUM; c.pin_pclk=PCLK_GPIO_NUM;
  c.pin_vsync=VSYNC_GPIO_NUM; c.pin_href=HREF_GPIO_NUM;
  c.pin_sccb_sda=SIOD_GPIO_NUM; c.pin_sccb_scl=SIOC_GPIO_NUM;
  c.pin_pwdn=PWDN_GPIO_NUM; c.pin_reset=RESET_GPIO_NUM;
  c.xclk_freq_hz=20000000;
  // Start at the BIGGEST size we'll ever use (HIRES_CEILING) so the PSRAM frame
  // buffers are allocated large enough; we immediately run it down to DRIVE size
  // below. This lets us shrink/grow at runtime without reallocating (the safe way).
  c.frame_size=HIRES_CEILING;
  c.pixel_format=PIXFORMAT_JPEG;
  c.grab_mode=CAMERA_GRAB_LATEST;              // always show the freshest frame
  c.fb_location=CAMERA_FB_IN_PSRAM;
  c.jpeg_quality=12;                           // 10=better/bigger, 20=worse/smaller
  c.fb_count=2;

  g_psram = psramFound();
  if (!g_psram) {                              // safety net if PSRAM is off
    c.frame_size=FRAMESIZE_QVGA; c.fb_location=CAMERA_FB_IN_DRAM; c.fb_count=1;
  }
  esp_err_t err = esp_camera_init(&c);
  if (err != ESP_OK) {
    Serial.printf("CAMERA INIT FAILED 0x%x — check camera_pins.h / PSRAM setting\n", err);
    return false;
  }
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_vflip(s, 1); s->set_hmirror(s, 0);  // flip if your image is upside down
    if (g_psram) { s->set_framesize(s, DRIVE_FRAMESIZE); g_curFrame = DRIVE_FRAMESIZE; }
    else         { g_curFrame = FRAMESIZE_QVGA; }   // no PSRAM -> stuck at the fallback
  }
  return true;
}

// Live image tuning — adjusted from the web controls without rebooting.
// brightness accepts -2..+2 (0 = normal); it just biases the picture.
void setBrightness(int v) {
  sensor_t *s = esp_camera_sensor_get();
  if (s) s->set_brightness(s, constrain(v, -2, 2));
}

// Signal strength (dBm) of the link. In AP mode that's how well the bot HEARS
// the phone; in STA mode it's how well the bot hears the router. ~ -45 strong,
// -75 getting weak, -85+ about to drop.
int linkRssi() {
#if WIFI_MODE == RECON_WIFI_AP
  wifi_sta_list_t list;
  if (esp_wifi_ap_get_sta_list(&list) == ESP_OK && list.num > 0)
    return list.sta[0].rssi;
  return 0;                                    // nobody connected
#else
  return WiFi.RSSI();
#endif
}

// Pick the biggest frame size the current signal can comfortably push, capped
// at HIRES_CEILING. Weak signal -> stay modest so the feed doesn't choke.
framesize_t chooseHiresFrame() {
  int r = linkRssi();
  framesize_t want;
  if      (r == 0)   want = DRIVE_FRAMESIZE;   // unknown / nobody connected
  else if (r >= -50) want = FRAMESIZE_UXGA;    // strong link
  else if (r >= -60) want = FRAMESIZE_SXGA;
  else if (r >= -70) want = FRAMESIZE_SVGA;
  else               want = DRIVE_FRAMESIZE;   // too weak for hi-res
  if ((int)want > (int)HIRES_CEILING) want = HIRES_CEILING;
  return want;
}

// HD button: on -> jump to the best size the link allows; off -> native feed.
void setHires(bool on) {
  g_hires = on;
  if (!g_psram) return;
  sensor_t *s = esp_camera_sensor_get();
  if (!s) return;
  framesize_t want = on ? chooseHiresFrame() : DRIVE_FRAMESIZE;
  if (want != g_curFrame && s->set_framesize(s, want) == 0) {
    g_curFrame = want;
    Serial.printf("Camera -> framesize %d (HD %s)\n", (int)want, on ? "on" : "off");
  }
}

// ---------------------------------------------------------------------------
//  HTTP: web app (port 80)
// ---------------------------------------------------------------------------
static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

// /control?joy=30,-80  &trim=-20 &bright=1 &light=1 &hires=1 &dance=1 &cmd=S
static esp_err_t control_handler(httpd_req_t *req) {
  char query[96] = {0};
  char val[24];
  bool tune = false;                            // trim changed this request?
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
    if (httpd_query_key_value(query, "trim", val, sizeof(val)) == ESP_OK) {
      g_trim = constrain(atoi(val), -100, 100); tune = true;
    }
    if (httpd_query_key_value(query, "bright", val, sizeof(val)) == ESP_OK)
      setBrightness(atoi(val));
    if (httpd_query_key_value(query, "light", val, sizeof(val)) == ESP_OK)
      setLight(atoi(val) != 0);
    if (httpd_query_key_value(query, "hires", val, sizeof(val)) == ESP_OK)
      setHires(atoi(val) != 0);
    if (httpd_query_key_value(query, "joy", val, sizeof(val)) == ESP_OK) {
      int x = 0, y = 0; sscanf(val, "%d,%d", &x, &y);
      if (!g_dancing) applyJoystick(x, y);      // ignore steering mid-dance
    }
    if (httpd_query_key_value(query, "cmd", val, sizeof(val)) == ESP_OK) {
      if (val[0] == 'S') motorsStop();          // 'S' = stop, 'P' = ping (keep-alive)
    }
    if (httpd_query_key_value(query, "dance", val, sizeof(val)) == ESP_OK) {
      if (atoi(val) && !g_dancing) danceRoutine();
    }
  }
  // Trim moved while driving -> re-mix now so it takes effect live.
  if (tune && !g_dancing) mixDrive();
  g_lastCmdMs = millis();                        // heard from phone -> reset failsafe
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, "ok", 2);
}

// /stat -> plain-text RSSI in dBm, polled by the HUD for the signal meter.
static esp_err_t stat_handler(httpd_req_t *req) {
  char b[16];
  int n = snprintf(b, sizeof(b), "%d", linkRssi());
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, b, n);
}

// ---------------------------------------------------------------------------
//  HTTP: MJPEG video stream (port 81)
// ---------------------------------------------------------------------------
#define PART_BOUNDARY "frameboundary"
static const char *STREAM_CT  = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *STREAM_BND = "\r\n--" PART_BOUNDARY "\r\n";
static const char *STREAM_PART= "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static esp_err_t stream_handler(httpd_req_t *req) {
  esp_err_t res = httpd_resp_set_type(req, STREAM_CT);
  if (res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  char part[64];
  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) { res = ESP_FAIL; break; }
    size_t hlen = snprintf(part, sizeof(part), STREAM_PART, fb->len);
    if ((res = httpd_resp_send_chunk(req, STREAM_BND, strlen(STREAM_BND))) != ESP_OK) { esp_camera_fb_return(fb); break; }
    if ((res = httpd_resp_send_chunk(req, part, hlen)) != ESP_OK)                      { esp_camera_fb_return(fb); break; }
    if ((res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len)) != ESP_OK)   { esp_camera_fb_return(fb); break; }
    esp_camera_fb_return(fb);
  }
  return res;
}

void startServers() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = 80;
  cfg.ctrl_port   = 32080;
  cfg.lru_purge_enable = true;
  httpd_uri_t index_uri   = { "/",        HTTP_GET, index_handler,   NULL };
  httpd_uri_t control_uri = { "/control", HTTP_GET, control_handler, NULL };
  httpd_uri_t stat_uri    = { "/stat",    HTTP_GET, stat_handler,    NULL };
  if (httpd_start(&control_httpd, &cfg) == ESP_OK) {
    httpd_register_uri_handler(control_httpd, &index_uri);
    httpd_register_uri_handler(control_httpd, &control_uri);
    httpd_register_uri_handler(control_httpd, &stat_uri);
  }
  cfg.server_port = 81;
  cfg.ctrl_port   = 32081;
  httpd_uri_t stream_uri  = { "/stream", HTTP_GET, stream_handler, NULL };
  if (httpd_start(&stream_httpd, &cfg) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

// ---------------------------------------------------------------------------
//  SETUP / LOOP
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nReconBot booting...");

  motorsBegin();  motorsStop();
  lightBegin();

#if USE_IMU
  Wire.begin(IMU_SDA, IMU_SCL);
#endif

  if (!cameraBegin()) {
    Serial.println("Continuing without camera — driving will still work.");
  }

#if WIFI_MODE == RECON_WIFI_AP
  WiFi.mode(WIFI_AP);
  // softAP(ssid, pass, channel, hidden, max_conn). hidden=1 keeps the SSID off
  // the air so it never appears in a phone's WiFi list (see AP_HIDDEN in config.h).
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_HIDDEN);
  Serial.printf("AP up%s. Join WiFi '%s', then open  http://192.168.4.1\n",
                AP_HIDDEN ? " (HIDDEN — add the network manually on your phone)" : "",
                AP_SSID);
#else
  WiFi.mode(WIFI_STA);
  WiFi.begin(STA_SSID, STA_PASS);
  Serial.print("Joining home WiFi");
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) { delay(250); Serial.print("."); }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("Connected. Open  http://%s\n", WiFi.localIP().toString().c_str());
  else
    Serial.println("WiFi failed — check STA_SSID/STA_PASS in config.h");
#endif

  startServers();
  g_lastCmdMs = millis();
  Serial.println("Ready. Drive safe.");
}

void loop() {
  // While the DANCE routine runs it owns the motors directly, so back off.
  if (!g_dancing) {
    // FAILSAFE: if the phone went quiet, stop the wheels.
    if (g_moving && (millis() - g_lastCmdMs > FAILSAFE_MS)) {
      motorsStop();
      Serial.println("Failsafe: no commands -> motors stopped.");
    }

    // Soft-start: ease the wheels toward their targets every tick.
    rampMotors();
  }
  delay(20);
}
