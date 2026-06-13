// ===========================================================================
//  web_index.h  —  The web app served to your iPhone
// ---------------------------------------------------------------------------
//  This whole file is one big text string stored in the ESP32's flash.
//  When your phone opens the bot's web page, the ESP32 sends this HTML.
//  It's a single page: live video in the background + tactical controls
//  on top, styled like a tactical drone HUD (green on black).
//
//  Controls: a virtual JOYSTICK (bottom-left) for proportional drive+steer,
//  sliders + toggles (bottom-right), and a DANCE button for fun.
//  The commands it sends (/control?joy=x,y etc.) must match reconbot.ino.
// ===========================================================================
#pragma once

const char INDEX_HTML[] PROGMEM = R"HTMLDOC(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover">
<!-- Add to Home Screen on iPhone -> launches with NO browser chrome (true fullscreen) -->
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="apple-mobile-web-app-title" content="ReconBot">
<title>ReconBot</title>
<style>
  :root { --grn:#39ff14; --grn-dim:#1f8b10; --bg:#05080a; }
  * { box-sizing:border-box; -webkit-user-select:none; user-select:none;
      -webkit-tap-highlight-color:transparent; }
  html,body { margin:0; height:100vh; height:100dvh; overflow:hidden;
      background:var(--bg); font-family:"Courier New",monospace; color:var(--grn); }
  /* Feed rotated 90deg. The element is sized in SWAPPED units (width=height,
     height=width) and centred, so once rotated it fills the screen. We use
     dynamic viewport units (dvh/dvw) so it stays full even as Safari's toolbars
     collapse; the vh/vw lines are the fallback for older browsers.
     If it's rotated the WRONG way, change rotate(90deg) to rotate(-90deg). */
  #video { position:fixed; top:50%; left:50%;
      width:100vh; height:100vw; width:100dvh; height:100dvw;
      transform:translate(-50%,-50%) rotate(90deg);
      object-fit:contain; background:#000; }   /* contain = whole frame, no crop/zoom
         (black bars on the sides). Change to cover if you'd rather fill+crop. */
  .hud { position:fixed; pointer-events:none; text-shadow:0 0 6px var(--grn-dim);
      font-size:13px; letter-spacing:1px; }
  #topbar { top:0; left:0; right:0; display:flex; justify-content:space-between;
      padding-top:max(10px, env(safe-area-inset-top));
      padding-bottom:10px;
      padding-left:max(14px, env(safe-area-inset-left));
      padding-right:max(14px, env(safe-area-inset-right)); }
  #sig { opacity:.9; }
  #status { color:#ff4040; }
  #status.ok { color:var(--grn); }
  /* iOS "add to home screen for fullscreen" hint (shown only when needed) */
  #hint { position:fixed; top:42px; left:50%; transform:translateX(-50%);
      max-width:82%; padding:9px 13px; border:1px solid var(--grn);
      background:rgba(5,8,10,.92); color:var(--grn); font-size:12px;
      line-height:1.4; border-radius:8px; text-align:center; z-index:5;
      pointer-events:auto; }
  .corner { position:fixed; width:26px; height:26px; border:2px solid var(--grn);
      opacity:.7; pointer-events:none; }
  #c1{top:8px;left:8px;border-right:0;border-bottom:0}
  #c2{top:8px;right:8px;border-left:0;border-bottom:0}
  #c3{bottom:8px;left:8px;border-right:0;border-top:0}
  #c4{bottom:8px;right:8px;border-left:0;border-top:0}

  /* Virtual joystick bottom-left. The max(...,env(safe-area-inset-*)) keeps it
     clear of the iPhone notch / home-indicator bar when running fullscreen. */
  #joy { position:fixed; width:150px; height:150px;
      bottom:max(30px, calc(env(safe-area-inset-bottom) + 12px));
      left:max(30px, calc(env(safe-area-inset-left) + 12px));
      border:2px solid var(--grn); border-radius:50%;
      background:rgba(57,255,20,.06); touch-action:none; }
  #knob { position:absolute; left:44px; top:44px; width:62px; height:62px;
      border:2px solid var(--grn); border-radius:50%;
      background:rgba(57,255,20,.18); box-shadow:0 0 14px var(--grn-dim);
      transition:left .05s linear, top .05s linear; }

  /* controls bottom-right (also nudged in past the safe-area insets) */
  #panel { position:fixed; width:150px;
      bottom:max(24px, calc(env(safe-area-inset-bottom) + 8px));
      right:max(18px, calc(env(safe-area-inset-right) + 8px));
      display:flex; flex-direction:column; gap:12px; pointer-events:auto; }
  .slab { font-size:11px; opacity:.85; margin-bottom:4px; }
  input[type=range]{ width:100%; accent-color:var(--grn); }
  #btns { display:flex; gap:8px; }
  .toggle { flex:1; border:2px solid var(--grn); color:var(--grn); text-align:center;
      padding:9px 0; border-radius:8px; font-size:11px; letter-spacing:1px;
      background:rgba(57,255,20,.08); }
  .toggle.on, .toggle:active { background:var(--grn); color:#000;
      box-shadow:0 0 14px var(--grn); }
</style>
</head>
<body>
  <img id="video" alt="connecting to camera...">
  <div id="c1" class="corner"></div><div id="c2" class="corner"></div>
  <div id="c3" class="corner"></div><div id="c4" class="corner"></div>

  <div id="topbar" class="hud">
    <span>&#9678; RECONBOT // DRONE FEED</span>
    <span><span id="sig">SIG ----</span> &nbsp; <span id="status">LINK: ...</span></span>
  </div>

  <div id="hint" style="display:none">For true fullscreen on iPhone:
    tap <b>Share</b> &#8594; <b>Add to Home Screen</b>, then open ReconBot
    from the new icon.<br>(tap to dismiss)</div>

  <div id="joy"><div id="knob"></div></div>

  <div id="panel">
    <div>
      <div class="slab">TRIM <span id="tmv">0</span></div>
      <input id="trim" type="range" min="-100" max="100" value="0">
    </div>
    <div>
      <div class="slab">BRIGHT <span id="brv">0</span></div>
      <input id="bright" type="range" min="-2" max="2" value="0">
    </div>
    <div id="btns">
      <div id="light" class="toggle">LIGHT</div>
      <div id="dance" class="toggle">DANCE</div>
      <div id="hd"    class="toggle">HD</div>
      <div id="fs" class="toggle" style="display:none">FULL</div>
    </div>
  </div>

<script>
  // Build URLs from whatever address the phone used to reach us.
  const host = location.hostname;          // 192.168.4.1 in AP mode
  const base = "http://" + host;           // control server (port 80)
  const stream = base + ":81/stream";      // MJPEG video server (port 81)
  const vid = document.getElementById("video");
  vid.src = stream;

  const statusEl = document.getElementById("status");
  function setLink(ok){ statusEl.textContent = ok ? "LINK: LIVE" : "LINK: LOST";
                        statusEl.className = ok ? "ok" : ""; }
  vid.onload  = ()=> setLink(true);
  vid.onerror = ()=> { setLink(false); setTimeout(()=>{ vid.src = stream + "?r=" + Date.now(); }, 1000); };

  // Fire a command at the bot. keepalive lets it send even on page hide.
  let lastAny = 0;
  function send(q){ lastAny = Date.now(); fetch(base + "/control?" + q, {keepalive:true}).catch(()=>{}); }

  // ---- Virtual joystick -------------------------------------------------
  const joy = document.getElementById("joy"), knob = document.getElementById("knob");
  const R = 75;                 // base radius (150/2)
  const LIM = 44;               // how far the knob centre may travel from middle
  let pending = null;           // newest "x,y" waiting to be sent (throttled)
  function centerKnob(){ knob.style.left = "44px"; knob.style.top = "44px"; }
  function moveTo(cx, cy){
    const r = joy.getBoundingClientRect();
    const dx = cx - (r.left + R), dy = cy - (r.top + R);
    const dist = Math.hypot(dx, dy);
    // Knob VISUAL is clamped to LIM so it stays inside the ring...
    let kx = dx, ky = dy;
    if (dist > LIM){ kx = dx / dist * LIM; ky = dy / dist * LIM; }
    knob.style.left = (44 + kx) + "px";
    knob.style.top  = (44 + ky) + "px";
    // ...but the INPUT is normalised against the full pad radius R, so you use
    // the whole pad's travel (finer control) instead of saturating at the ring.
    const clamp = v => Math.max(-100, Math.min(100, Math.round(v)));
    pending = clamp(dx / R * 100) + "," + clamp(-dy / R * 100);
  }
  function release(){ centerKnob(); pending = null; send("joy=0,0"); }

  // touch (phone)
  let touchId = null;
  joy.addEventListener("touchstart", e=>{ e.preventDefault();
    const t = e.changedTouches[0]; touchId = t.identifier; moveTo(t.clientX, t.clientY); }, {passive:false});
  joy.addEventListener("touchmove", e=>{ e.preventDefault();
    for (const t of e.changedTouches) if (t.identifier === touchId) moveTo(t.clientX, t.clientY); }, {passive:false});
  const endTouch = e=>{ e.preventDefault();
    for (const t of e.changedTouches) if (t.identifier === touchId){ touchId = null; release(); } };
  joy.addEventListener("touchend", endTouch, {passive:false});
  joy.addEventListener("touchcancel", endTouch, {passive:false});
  // mouse (desktop testing)
  let mouseDown = false;
  joy.addEventListener("mousedown", e=>{ mouseDown = true; moveTo(e.clientX, e.clientY); });
  window.addEventListener("mousemove", e=>{ if (mouseDown) moveTo(e.clientX, e.clientY); });
  window.addEventListener("mouseup", ()=>{ if (mouseDown){ mouseDown = false; release(); } });

  // Pump: flush the latest joystick value ~10x/sec; ping if we've gone quiet so
  // the bot's failsafe knows we're still here.
  setInterval(()=>{
    if (pending !== null){ send("joy=" + pending); pending = null; }
    else if (Date.now() - lastAny > 350){ send("cmd=P"); }
  }, 100);

  // ---- Sliders ----------------------------------------------------------
  const tm = document.getElementById("trim"), tmv = document.getElementById("tmv");
  tm.addEventListener("input", ()=>{ tmv.textContent = tm.value; send("trim=" + tm.value); });
  const br = document.getElementById("bright"), brv = document.getElementById("brv");
  br.addEventListener("input", ()=>{ brv.textContent = br.value; send("bright=" + br.value); });

  // ---- Toggles + DANCE --------------------------------------------------
  const lt = document.getElementById("light"); let lightOn = false;
  lt.addEventListener("click", ()=>{ lightOn = !lightOn;
      lt.classList.toggle("on", lightOn); send("light=" + (lightOn ? 1 : 0)); });
  const dc = document.getElementById("dance");
  dc.addEventListener("click", ()=>{ send("dance=1"); });
  // HD: jump to the best resolution the signal allows; untick -> back to 640.
  const hd = document.getElementById("hd"); let hdOn = false;
  hd.addEventListener("click", ()=>{ hdOn = !hdOn;
      hd.classList.toggle("on", hdOn); send("hires=" + (hdOn ? 1 : 0)); });

  // ---- Fullscreen -------------------------------------------------------
  // iOS Safari has NO Fullscreen API for non-video elements, so the only way
  // to lose the browser chrome is "Add to Home Screen" (runs standalone).
  const isIOS = /iphone|ipad|ipod/i.test(navigator.userAgent)
             || (navigator.platform === "MacIntel" && navigator.maxTouchPoints > 1);
  const standalone = navigator.standalone === true
             || matchMedia("(display-mode: standalone)").matches;
  // Show the "add to home screen" hint only on iOS when NOT already standalone.
  if (isIOS && !standalone){
    const hint = document.getElementById("hint");
    hint.style.display = "block";
    hint.addEventListener("click", ()=> hint.style.display = "none");
    setTimeout(()=> hint.style.display = "none", 9000);
  }
  // On platforms that DO support real fullscreen (Android/desktop), show FULL.
  const fs = document.getElementById("fs");
  if (document.documentElement.requestFullscreen){
    fs.style.display = "";
    fs.addEventListener("click", ()=>{
      if (document.fullscreenElement) document.exitFullscreen();
      else document.documentElement.requestFullscreen().catch(()=>{});
    });
  }

  // ---- Signal meter -----------------------------------------------------
  const sig = document.getElementById("sig");
  const F = "▮", E = "▯";    // filled / empty bar blocks
  function bars(d){
    if (!d) return E + E + E + E;
    if (d >= -55) return F + F + F + F;
    if (d >= -65) return F + F + F + E;
    if (d >= -75) return F + F + E + E;
    if (d >= -85) return F + E + E + E;
    return E + E + E + E;
  }
  async function poll(){
    try { const r = await fetch(base + "/stat"); const d = parseInt(await r.text());
          sig.textContent = "SIG " + bars(d) + " " + d; }
    catch(e){ sig.textContent = "SIG " + (E+E+E+E); }
  }
  setInterval(poll, 1000); poll();
</script>
</body>
</html>
)HTMLDOC";
