/* web_ui_v7.h – Smart Classroom Dashboard v7
   Changes vs v6:
     • SD card / audio panel completely removed
     • Refined dark-glass aesthetic: sharper, denser, more professional
     • PIR door timer countdown shown in UI
     • Reed bars and motor grid preserved + improved
*/
#pragma once

const char DASHBOARD_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Classroom</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Space+Mono:wght@400;700&family=DM+Sans:wght@300;400;500;700&display=swap" rel="stylesheet">
<style>
/* ── Variables ─────────────────────────────────────────────── */
:root{
  --bg:       #060d17;
  --surface:  #0a1628;
  --surface2: #0f1f38;
  --border:   rgba(255,255,255,.07);
  --border2:  rgba(255,255,255,.12);
  --accent:   #2563eb;
  --accent2:  #3b82f6;
  --cyan:     #22d3ee;
  --green:    #4ade80;
  --red:      #f87171;
  --orange:   #fb923c;
  --yellow:   #fbbf24;
  --purple:   #a78bfa;
  --text:     #e2e8f0;
  --muted:    #475569;
  --muted2:   #64748b;
  --mono:     'Space Mono', monospace;
  --sans:     'DM Sans', system-ui, sans-serif;
}

/* ── Reset ──────────────────────────────────────────────────── */
*{ box-sizing:border-box; margin:0; padding:0 }
body{
  font-family:var(--sans); background:var(--bg); color:var(--text);
  min-height:100vh;
  background-image:
    radial-gradient(ellipse 80% 50% at 20% -10%, rgba(37,99,235,.12) 0%, transparent 60%),
    radial-gradient(ellipse 60% 40% at 80% 110%, rgba(34,211,238,.07) 0%, transparent 55%);
}

/* ── Header ─────────────────────────────────────────────────── */
header{
  display:flex; align-items:center; justify-content:space-between;
  flex-wrap:wrap; gap:10px;
  padding:14px 24px;
  background:rgba(10,22,40,.85);
  border-bottom:1px solid var(--border2);
  backdrop-filter:blur(12px);
  position:sticky; top:0; z-index:100;
}
.logo{ display:flex; align-items:center; gap:12px }
.logo-mark{
  width:38px; height:38px; border-radius:10px;
  background:linear-gradient(135deg, #1d4ed8, #0ea5e9);
  display:flex; align-items:center; justify-content:center;
  font-size:1.2rem; flex-shrink:0;
  box-shadow:0 0 20px rgba(37,99,235,.4);
}
.logo-text h1{ font-size:.95rem; font-weight:700; letter-spacing:.01em; color:var(--text) }
.logo-text p { font-size:.7rem; color:var(--muted2); margin-top:1px; font-weight:300 }

.hdr-right{ display:flex; align-items:center; gap:10px; flex-wrap:wrap }

/* Connection pill */
.ws-pill{
  display:flex; align-items:center; gap:6px;
  padding:6px 13px; border-radius:20px; font-size:.72rem;
  background:rgba(255,255,255,.04);
  border:1px solid var(--border2);
  font-family:var(--mono);
}
#ws-dot{ width:7px; height:7px; border-radius:50%; background:var(--red); flex-shrink:0; transition:background .3s }
#ws-dot.on{ background:var(--green); box-shadow:0 0 8px var(--green) }

/* Master button */
.master{
  padding:9px 20px; border-radius:10px; border:none; cursor:pointer;
  font-size:.8rem; font-weight:700; letter-spacing:.04em;
  display:flex; align-items:center; gap:8px;
  transition:all .2s; font-family:var(--sans);
}
.master.is-off{
  background:rgba(37,99,235,.15); color:var(--accent2);
  border:1px solid rgba(37,99,235,.35);
}
.master.is-off:hover{ background:rgba(37,99,235,.3); border-color:var(--accent2); transform:translateY(-1px) }
.master.is-on{
  background:linear-gradient(135deg,#dc2626,#991b1b);
  color:#fff; border:1px solid rgba(220,38,38,.5);
  box-shadow:0 0 20px rgba(220,38,38,.3);
}
.master.is-on:hover{ box-shadow:0 0 28px rgba(220,38,38,.5); transform:translateY(-1px) }
.master-dot{ width:8px; height:8px; border-radius:50%; flex-shrink:0 }
.is-on  .master-dot{ background:#fca5a5; animation:pulse 1.5s infinite }
.is-off .master-dot{ background:var(--muted) }
@keyframes pulse{ 0%,100%{opacity:1;transform:scale(1)} 50%{opacity:.5;transform:scale(.8)} }

/* ── Layout grid ────────────────────────────────────────────── */
.grid{
  display:grid;
  grid-template-columns:repeat(12, 1fr);
  gap:12px; padding:16px 24px;
  max-width:1400px; margin:0 auto;
}
.col4 { grid-column:span 4 }
.col6 { grid-column:span 6 }
.col8 { grid-column:span 8 }
.col12{ grid-column:span 12 }
@media(max-width:1100px){ .col4,.col6,.col8{ grid-column:span 6 } }
@media(max-width:700px) { .col4,.col6,.col8,.col12{ grid-column:span 12 } }

/* ── Cards ──────────────────────────────────────────────────── */
.card{
  background:var(--surface);
  border:1px solid var(--border2);
  border-radius:14px; padding:18px;
  position:relative; overflow:hidden;
}
.card::before{
  content:''; position:absolute; inset:0; border-radius:14px;
  background:linear-gradient(135deg, rgba(255,255,255,.025) 0%, transparent 60%);
  pointer-events:none;
}
.card-title{
  font-size:.68rem; font-weight:600; color:var(--muted2);
  text-transform:uppercase; letter-spacing:.12em;
  margin-bottom:14px; display:flex; align-items:center; gap:7px;
}
.card-title .ct-icon{ font-size:.95rem }

/* ── Sensor values ──────────────────────────────────────────── */
.sensor-row{ display:flex; gap:10px }
.sensor-box{
  flex:1; background:var(--surface2); border-radius:10px;
  padding:14px 12px; text-align:center; border:1px solid var(--border);
}
.sensor-val{ font-size:2.1rem; font-weight:700; line-height:1; font-family:var(--mono) }
.sensor-val.temp{ color:var(--orange) }
.sensor-val.hum { color:var(--cyan)   }
.sensor-lbl{ font-size:.63rem; color:var(--muted2); margin-top:5px;
  text-transform:uppercase; letter-spacing:.08em; font-weight:500 }

/* PIR notice */
.pir-notice{
  display:none; margin-top:12px;
  padding:8px 12px; background:rgba(251,146,60,.08);
  border:1px solid rgba(251,146,60,.25); border-radius:8px;
  font-size:.74rem; color:var(--orange);
  align-items:center; gap:8px;
}
.pir-notice.active{ display:flex }
#pir-countdown{
  font-family:var(--mono); font-weight:700; font-size:.82rem;
  background:rgba(251,146,60,.15); border-radius:5px;
  padding:1px 7px; margin-left:auto;
}

/* Status info row */
.info-row{
  display:flex; align-items:center; gap:10px; flex-wrap:wrap;
  margin-top:12px; font-size:.72rem; color:var(--muted2);
  padding:8px 10px; background:var(--surface2);
  border-radius:8px; border:1px solid var(--border);
}
.info-row b{ color:var(--text) }
.dot{ width:6px; height:6px; border-radius:50%; flex-shrink:0 }
.dot-on { background:var(--green); box-shadow:0 0 5px var(--green) }
.dot-off{ background:var(--red) }

/* ── Relay controls ─────────────────────────────────────────── */
.ctrl-grid{ display:grid; grid-template-columns:1fr 1fr; gap:8px }

.ctrl-btn{
  padding:10px 8px; border:none; border-radius:9px; cursor:pointer;
  font-size:.76rem; font-weight:700; font-family:var(--sans);
  display:flex; align-items:center; justify-content:center; gap:6px;
  transition:all .12s; border:1px solid transparent;
}
.ctrl-btn:active{ transform:scale(.95) }

.btn-on {background:rgba(74,222,128,.12);color:var(--green);border-color:rgba(74,222,128,.25)}
.btn-on:hover{background:rgba(74,222,128,.22);border-color:rgba(74,222,128,.5)}
.btn-off{background:rgba(248,113,113,.1);color:var(--red);border-color:rgba(248,113,113,.22)}
.btn-off:hover{background:rgba(248,113,113,.2);border-color:rgba(248,113,113,.45)}
.btn-open{background:rgba(37,99,235,.12);color:var(--accent2);border-color:rgba(37,99,235,.3)}
.btn-open:hover{background:rgba(37,99,235,.25);border-color:var(--accent2)}
.btn-close{background:rgba(251,146,60,.1);color:var(--orange);border-color:rgba(251,146,60,.25)}
.btn-close:hover{background:rgba(251,146,60,.2);border-color:var(--orange)}
.btn-stop{background:rgba(167,139,250,.1);color:var(--purple);border-color:rgba(167,139,250,.25)}
.btn-stop:hover{background:rgba(167,139,250,.2);border-color:var(--purple)}
.btn-red{background:rgba(248,113,113,.1);color:var(--red);border-color:rgba(248,113,113,.22)}
.btn-red:hover{background:rgba(248,113,113,.2);border-color:rgba(248,113,113,.5)}

/* ── Motor grid ─────────────────────────────────────────────── */
.mtr-grid{
  display:grid;
  grid-template-columns:repeat(4,1fr);
  gap:8px;
}
@media(max-width:700px){ .mtr-grid{ grid-template-columns:1fr 1fr } }

.mtr-card{
  background:var(--surface2); border-radius:10px;
  padding:12px; border:1px solid var(--border);
  transition:border-color .2s;
}
.mtr-card.is-open{ border-color:rgba(37,99,235,.35) }
.mtr-name{
  font-size:.68rem; font-weight:700; color:var(--muted2);
  text-transform:uppercase; letter-spacing:.1em; margin-bottom:6px
}
.mtr-state{
  font-size:.82rem; font-weight:700; margin-bottom:8px;
  font-family:var(--mono);
}
.mtr-state.open  { color:var(--green) }
.mtr-state.closed{ color:var(--muted2) }
.mtr-btns{ display:grid; grid-template-columns:1fr 1fr; gap:4px }
.mtr-btns .ctrl-btn{ padding:6px 4px; font-size:.68rem }

/* Motor status bar */
.motor-status-bar{
  display:flex; align-items:center; justify-content:space-between;
  gap:10px; margin-top:10px;
  padding:8px 12px; background:var(--surface2);
  border:1px solid var(--border); border-radius:8px;
  font-size:.72rem;
}
.motor-status-bar b{ font-family:var(--mono); color:var(--cyan) }

/* ── Reed confirmation ──────────────────────────────────────── */
.reed-section{ margin-top:14px }
.reed-section-title{
  font-size:.67rem; font-weight:600; color:var(--muted);
  text-transform:uppercase; letter-spacing:.12em; margin-bottom:8px
}
.reed-row{ display:flex; gap:8px }

.reed-bar{
  flex:1; border-radius:10px; padding:11px 14px;
  display:flex; align-items:center; justify-content:space-between; gap:10px;
  border:1px solid var(--border); transition:all .3s;
  background:var(--surface2);
}
.reed-bar.r-closed{
  background:rgba(74,222,128,.06); border-color:rgba(74,222,128,.25)
}
.reed-bar.stuck{
  background:rgba(248,113,113,.07); border-color:rgba(248,113,113,.4);
  animation:reedpulse 1.2s ease-in-out infinite
}
@keyframes reedpulse{
  0%,100%{ box-shadow:none; border-color:rgba(248,113,113,.4) }
  50%{ box-shadow:0 0 18px rgba(248,113,113,.3); border-color:var(--red) }
}
.reed-left{ display:flex; align-items:center; gap:10px }
.reed-indicator{
  width:10px; height:10px; border-radius:50%; flex-shrink:0; transition:all .3s
}
.reed-bar.idle     .reed-indicator{ background:var(--muted) }
.reed-bar.r-closed .reed-indicator{ background:var(--green); box-shadow:0 0 8px var(--green) }
.reed-bar.stuck    .reed-indicator{
  background:var(--red); box-shadow:0 0 8px var(--red);
  animation:pulse .7s infinite
}
.reed-lbl{ font-size:.76rem; font-weight:700 }
.reed-bar.idle     .reed-lbl{ color:var(--muted2) }
.reed-bar.r-closed .reed-lbl{ color:var(--green) }
.reed-bar.stuck    .reed-lbl{ color:var(--red) }
.reed-sub{ font-size:.63rem; margin-top:2px }
.reed-bar.idle     .reed-sub{ color:var(--muted) }
.reed-bar.r-closed .reed-sub{ color:rgba(74,222,128,.6) }
.reed-bar.stuck    .reed-sub{ color:rgba(248,113,113,.7) }

.reed-force{
  display:none; padding:6px 12px; border:none; border-radius:7px;
  cursor:pointer; font-size:.7rem; font-weight:700;
  background:rgba(248,113,113,.15); color:var(--red);
  border:1px solid rgba(248,113,113,.35); white-space:nowrap;
  transition:all .15s; flex-shrink:0;
}
.reed-force:hover{ background:rgba(248,113,113,.3); border-color:var(--red) }
.reed-force:active{ transform:scale(.95) }
.reed-bar.stuck .reed-force{ display:block }

/* ── Camera ─────────────────────────────────────────────────── */
.cam-wrap{
  background:#000; border-radius:10px; overflow:hidden;
  aspect-ratio:4/3; position:relative;
  border:1px solid var(--border);
}
.cam-wrap img{ width:100%; height:100%; object-fit:cover; display:block }
.live-badge{
  position:absolute; top:8px; left:8px;
  background:rgba(220,38,38,.85); color:#fff;
  font-size:.62rem; font-weight:700;
  padding:3px 9px; border-radius:20px;
  display:flex; align-items:center; gap:5px;
  letter-spacing:.06em; font-family:var(--mono);
  backdrop-filter:blur(4px);
}
.live-badge::before{
  content:''; width:5px; height:5px; border-radius:50%;
  background:#fff; animation:pulse .9s infinite
}
.cam-ctrl{ display:flex; gap:8px; margin-top:9px }
.cam-ctrl .ctrl-btn{ flex:1; padding:9px }

/* ── Scrollbar ──────────────────────────────────────────────── */
::-webkit-scrollbar{ width:5px }
::-webkit-scrollbar-track{ background:transparent }
::-webkit-scrollbar-thumb{ background:var(--border2); border-radius:3px }
</style>
</head>
<body>

<!-- ── Header ────────────────────────────────────────────── -->
<header>
  <div class="logo">
    <div class="logo-mark">🏫</div>
    <div class="logo-text">
      <h1>Smart Classroom</h1>
      <p>Automation Control System</p>
    </div>
  </div>
  <div class="hdr-right">
    <div class="ws-pill">
      <span id="ws-dot"></span>
      <span id="ws-txt">Connecting…</span>
    </div>
    <button class="master is-off" id="master-btn" onclick="toggleMaster()">
      <span class="master-dot"></span>
      <span id="master-lbl">ACTIVATE CLASSROOM</span>
    </button>
  </div>
</header>

<!-- ── Main grid ─────────────────────────────────────────── -->
<div class="grid">

  <!-- Environment sensors -->
  <div class="card col4">
    <div class="card-title"><span class="ct-icon">🌡️</span>Environment</div>
    <div class="sensor-row">
      <div class="sensor-box">
        <div class="sensor-val temp" id="sv-temp">--.-</div>
        <div class="sensor-lbl">Temperature °C</div>
      </div>
      <div class="sensor-box">
        <div class="sensor-val hum" id="sv-hum">--</div>
        <div class="sensor-lbl">Humidity %</div>
      </div>
    </div>
    <div class="pir-notice" id="pir-notice">
      <span>🚶 Motion — door auto-opened</span>
      <span id="pir-countdown">5s</span>
    </div>
    <div class="info-row" style="margin-top:10px">
      <span style="font-family:var(--mono);font-size:.68rem;color:var(--muted2)" id="ip-label">—</span>
    </div>
  </div>

  <!-- Electrical controls -->
  <div class="card col4">
    <div class="card-title"><span class="ct-icon">⚡</span>Electrical</div>
    <div class="ctrl-grid">
      <button class="ctrl-btn btn-on"  onclick="send('LIGHT_ON')">💡 Light ON</button>
      <button class="ctrl-btn btn-off" onclick="send('LIGHT_OFF')">💡 Light OFF</button>
      <button class="ctrl-btn btn-on"  onclick="send('FAN_ON')">🌀 Fan ON</button>
      <button class="ctrl-btn btn-off" onclick="send('FAN_OFF')">🌀 Fan OFF</button>
    </div>
    <div class="info-row">
      <span class="dot" id="dot-l"></span>
      <span>Light: <b id="st-light">--</b></span>
      <span style="margin-left:8px" class="dot" id="dot-f"></span>
      <span>Fan: <b id="st-fan">--</b></span>
    </div>
  </div>

  <!-- Camera -->
  <div class="card col4">
    <div class="card-title"><span class="ct-icon">📷</span>CCTV Live</div>
    <div class="cam-wrap">
      <img id="cam-img" src="" alt="Connecting…" onerror="scheduleReload()">
      <div class="live-badge">LIVE</div>
    </div>
    <div class="cam-ctrl">
      <button class="ctrl-btn btn-off" id="flash-btn" onclick="toggleFlash()">🔦 Flash</button>
      <button class="ctrl-btn btn-open" style="max-width:48px" onclick="reloadStream()">↺</button>
    </div>
  </div>

  <!-- Motors + Reed -->
  <div class="card col12">
    <div class="card-title"><span class="ct-icon">⚙️</span>Actuator Control</div>

    <div class="mtr-grid" id="mtr-grid"><!-- built by JS --></div>

    <!-- Reed confirmation -->
    <div class="reed-section">
      <div class="reed-section-title">Reed Switch Confirmation</div>
      <div class="reed-row">

        <div class="reed-bar idle" id="reed1-bar">
          <div class="reed-left">
            <span class="reed-indicator"></span>
            <div>
              <div class="reed-lbl" id="reed1-lbl">WIN 1 — OPEN</div>
              <div class="reed-sub" id="reed1-sub">Awaiting close confirmation</div>
            </div>
          </div>
          <button class="reed-force" id="reed1-force" onclick="forceClose(1)">⚠ Force Close W1</button>
        </div>

        <div class="reed-bar idle" id="reed2-bar">
          <div class="reed-left">
            <span class="reed-indicator"></span>
            <div>
              <div class="reed-lbl" id="reed2-lbl">WIN 2 — OPEN</div>
              <div class="reed-sub" id="reed2-sub">Awaiting close confirmation</div>
            </div>
          </div>
          <button class="reed-force" id="reed2-force" onclick="forceClose(2)">⚠ Force Close W2</button>
        </div>

      </div>
    </div>

    <!-- Motor status bar -->
    <div class="motor-status-bar">
      <span style="color:var(--muted2);font-size:.72rem">Motor status:</span>
      <b id="st-motor">IDLE</b>
      <button class="ctrl-btn btn-stop" style="width:auto;padding:7px 16px;margin-left:auto"
              onclick="send('ALL_STOP')">■ ALL STOP</button>
    </div>
  </div>

</div><!-- /grid -->

<script>
let ws2, flashOn=false, streamUrl='', flashUrl='', classOn=false;
let pirTimer=null, pirCountdown=5;

const MOTORS=[
  {id:'win1',    label:'Window 1', openCmd:'WIN1_CLOSE',    closeCmd:'WIN1_OPEN'},
  {id:'win2',    label:'Window 2', openCmd:'WIN2_OPEN',    closeCmd:'WIN2_CLOSE'},
  {id:'curtain', label:'Curtain',  openCmd:'CURTAIN_OPEN', closeCmd:'CURTAIN_CLOSE'},
  {id:'door',    label:'Door',     openCmd:'DOOR_CLOSE',    closeCmd:'DOOR_OPEN'},
];

(function buildGrid(){
  document.getElementById('mtr-grid').innerHTML = MOTORS.map(m=>`
    <div class="mtr-card" id="mc-${m.id}">
      <div class="mtr-name">${m.label}</div>
      <div class="mtr-state closed" id="ms-${m.id}">CLOSED</div>
      <div class="mtr-btns">
        <button class="ctrl-btn btn-open" onclick="send('${m.openCmd}')">▲ Open</button>
        <button class="ctrl-btn btn-close" onclick="send('${m.closeCmd}')">▼ Close</button>
      </div>
    </div>`).join('');
})();

// ── WebSocket ───────────────────────────────────────────────
function connect(){
  ws2 = new WebSocket('ws://'+location.host+'/ws');
  ws2.onopen  = ()=>{ wsDot(true); fetchCam(); fetchState(); };
  ws2.onclose = ()=>{ wsDot(false); setTimeout(connect,2500); };
  ws2.onmessage = e=>applyState(JSON.parse(e.data));
}
function wsDot(on){
  document.getElementById('ws-dot').className=on?'on':'';
  document.getElementById('ws-txt').textContent=on?'Connected':'Reconnecting…';
}
function send(cmd,extra={}){
  if(ws2&&ws2.readyState===WebSocket.OPEN)
    ws2.send(JSON.stringify({cmd,...extra}));
}
function fetchState(){
  fetch('/api/state').then(r=>r.json()).then(applyState).catch(()=>{});
}

// ── PIR countdown ───────────────────────────────────────────
function startPirCountdown(){
  clearInterval(pirTimer);
  pirCountdown=5;
  set('pir-countdown','5s');
  pirTimer=setInterval(()=>{
    pirCountdown--;
    set('pir-countdown', pirCountdown>0 ? pirCountdown+'s' : '…');
    if(pirCountdown<=0) clearInterval(pirTimer);
  },1000);
}

// ── Reed helper ─────────────────────────────────────────────
function setReed(n, state){
  const bar=document.getElementById('reed'+n+'-bar');
  const lbl=document.getElementById('reed'+n+'-lbl');
  const sub=document.getElementById('reed'+n+'-sub');
  if(!bar) return;
  bar.className='reed-bar '+(state==='closed'?'r-closed':state);
  if(state==='closed'){
    lbl.textContent='WIN '+n+' — CONFIRMED ✓';
    sub.textContent='Reed switch: magnet detected, window sealed';
  } else if(state==='stuck'){
    lbl.textContent='WIN '+n+' — NOT CLOSED ⚠';
    sub.textContent='Reed not triggered — window may be obstructed';
  } else {
    lbl.textContent='WIN '+n+' — OPEN';
    sub.textContent='Close window to confirm with reed sensor';
  }
}
function forceClose(n){ send('WIN'+n+'_FORCE_CLOSE'); setReed(n,'idle'); }

// ── Apply full state ─────────────────────────────────────────
function applyState(d){
  set('sv-temp', d.temp    !==undefined ? d.temp.toFixed(1)    :'--.-');
  set('sv-hum',  d.humidity!==undefined ? d.humidity.toFixed(0):'--');

  set('st-light', d.light?'ON':'OFF');
  set('st-fan',   d.fan  ?'ON':'OFF');
  dotSet('dot-l', d.light);
  dotSet('dot-f', d.fan);

  MOTORS.forEach(m=>{
    const ms=document.getElementById('ms-'+m.id);
    const mc=document.getElementById('mc-'+m.id);
    const open=!!d[m.id];
    if(ms){ ms.textContent=open?'OPEN':'CLOSED'; ms.className='mtr-state '+(open?'open':'closed'); }
    if(mc){ mc.className='mtr-card '+(open?'is-open':''); }
  });
  set('st-motor', d.motor||'IDLE');

  classOn=!!d.classOn; updateMaster();

  // PIR
  const pn=document.getElementById('pir-notice');
  if(pn){
    const wasActive=pn.classList.contains('active');
    pn.className='pir-notice'+(d.pir?' active':'');
    if(d.pir && !wasActive) startPirCountdown();
  }

  // Reed bars
  if(d.win1Alert)                            setReed(1,'stuck');
  else if(d.reed1Open===false && !d.win1)    setReed(1,'closed');
  else                                        setReed(1,'idle');

  if(d.win2Alert)                            setReed(2,'stuck');
  else if(d.reed2Open===false && !d.win2)    setReed(2,'closed');
  else                                        setReed(2,'idle');

  // IP
  if(d.ip) set('ip-label', '🌐 '+d.ip);
}

function set(id,v){ const e=document.getElementById(id); if(e) e.textContent=v; }
function dotSet(id,on){ const e=document.getElementById(id); if(e) e.className='dot '+(on?'dot-on':'dot-off'); }

function toggleMaster(){ send(classOn?'MASTER_OFF':'MASTER_ON'); }
function updateMaster(){
  const btn=document.getElementById('master-btn');
  const lbl=document.getElementById('master-lbl');
  if(!btn||!lbl) return;
  if(classOn){
    btn.className='master is-on';
    lbl.textContent='CLASSROOM ACTIVE — DEACTIVATE';
  } else {
    btn.className='master is-off';
    lbl.textContent='ACTIVATE CLASSROOM';
  }
}

// ── Camera ──────────────────────────────────────────────────
let reloadTmr=null;
function fetchCam(){
  fetch('/api/cam').then(r=>r.json()).then(d=>{
    streamUrl=d.stream; flashUrl=d.flash;
    document.getElementById('cam-img').src=streamUrl;
  }).catch(()=>{});
}
function reloadStream(){
  const img=document.getElementById('cam-img');
  img.src='';
  setTimeout(()=>{ img.src=streamUrl+'?t='+Date.now(); },350);
}
function scheduleReload(){ clearTimeout(reloadTmr); reloadTmr=setTimeout(reloadStream,3000); }
function toggleFlash(){
  flashOn=!flashOn;
  const btn=document.getElementById('flash-btn');
  btn.textContent=flashOn?'🔦 Flash ON':'🔦 Flash';
  btn.className='ctrl-btn '+(flashOn?'btn-on':'btn-off');
  fetch(flashUrl+'?intensity='+(flashOn?200:0)).catch(()=>{});
}

connect();
</script>
</body>
</html>
)rawhtml";
