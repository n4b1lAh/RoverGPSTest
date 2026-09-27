/*
 * Mars Rover Ground Station // 100% OFFLINE TACTICAL RADAR
 * RadioLink SE100 GPS + ESP32
 *
 * Zero Internet / Zero CDN Dependency — Runs 100% from ESP32 Flash
 * Local 1m/2m Metric Precision Grid + Relative Odometry
 *
 * Wi-Fi AP: MarsRover-GroundStation (Pass: rover1234)
 * URL:      http://192.168.4.1
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// ------- Hardware Config -------
static const int GPS_RX_PIN = 16;  // Orange wire (SE100 TX) -> ESP32 RX2
static const int GPS_TX_PIN = 17;  // White wire  (SE100 RX) -> ESP32 TX2
static const uint32_t GPS_BAUD = 38400; // RadioLink SE100 baud rate

const char* AP_SSID = "MarsRover-GroundStation";
const char* AP_PASS = "rover1234";

TinyGPSPlus gps;
HardwareSerial GPSSerial(2);
WebServer server(80);

uint32_t totalChars = 0;
unsigned long lastSerialPrint = 0;
char lastRawNMEA[128] = "$GNGLL,0,0,0,V*00";

// 100% SELF-CONTAINED HTML5 / CSS / CANVAS APP (NO EXTERNAL SCRIPTS / NO INTERNET REQUIRED)
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>MARS ROVER // TACTICAL RADAR</title>
  <style>
    :root {
      --bg: #070b12;
      --panel: #0d1524;
      --card: #131f33;
      --border: #1e3352;
      --cyan: #00f0ff;
      --green: #10b981;
      --amber: #f59e0b;
      --red: #ef4444;
      --text: #e2e8f0;
      --dim: #8493a8;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Consolas, monospace, sans-serif; }
    body { background: var(--bg); color: var(--text); height: 100vh; display: flex; flex-direction: column; overflow: hidden; }

    /* Top Navigation Bar */
    header {
      background: var(--panel);
      border-bottom: 1px solid var(--border);
      padding: 10px 18px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      z-index: 10;
    }
    .brand { display: flex; align-items: center; gap: 12px; }
    .brand-badge {
      background: linear-gradient(135deg, #00f0ff, #3b82f6);
      color: #000; font-weight: 900; font-size: 14px;
      width: 28px; height: 28px; border-radius: 6px;
      display: flex; align-items: center; justify-content: center;
    }
    .brand-title { font-size: 15px; font-weight: 700; letter-spacing: 1px; color: #fff; }
    .brand-sub { font-size: 10px; color: var(--cyan); letter-spacing: 2px; }

    .header-right { display: flex; align-items: center; gap: 14px; }
    .status-pill {
      display: flex; align-items: center; gap: 8px;
      background: rgba(16, 185, 129, 0.1); border: 1px solid var(--green);
      padding: 4px 12px; border-radius: 20px; font-size: 11px; font-weight: 700; color: var(--green);
    }
    .status-dot { width: 8px; height: 8px; border-radius: 50%; background: var(--green); animation: pulse 1.4s infinite; }
    @keyframes pulse { 0% { transform: scale(0.9); opacity: 0.4; } 50% { transform: scale(1.3); opacity: 1; } 100% { transform: scale(0.9); opacity: 0.4; } }

    /* Main Container */
    .main-view { display: flex; flex: 1; height: calc(100vh - 54px); }

    /* Left Telemetry Column */
    .sidebar {
      width: 350px;
      background: var(--panel);
      border-right: 1px solid var(--border);
      padding: 14px;
      display: flex;
      flex-direction: column;
      gap: 10px;
      overflow-y: auto;
    }
    .card {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 10px 14px;
      position: relative;
    }
    .card-accent { position: absolute; left: 0; top: 0; bottom: 0; width: 3px; background: var(--cyan); border-radius: 8px 0 0 8px; }
    .card-label { font-size: 10px; color: var(--dim); letter-spacing: 1.2px; text-transform: uppercase; margin-bottom: 4px; }
    .card-val { font-size: 18px; font-weight: 700; font-family: monospace; color: #fff; }
    .card-meta { font-size: 11px; color: var(--dim); margin-top: 4px; display: flex; justify-content: space-between; }
    .row-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }

    /* Controls */
    .btn-row { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-top: auto; padding-top: 8px; }
    button {
      background: rgba(0, 240, 255, 0.12); border: 1px solid var(--cyan);
      color: #fff; padding: 9px 12px; border-radius: 6px; font-size: 11px; font-weight: 700;
      cursor: pointer; transition: 0.2s;
    }
    button:hover { background: var(--cyan); color: #000; }
    button.btn-warn { background: rgba(245, 158, 11, 0.12); border-color: var(--amber); color: #fbbf24; }
    button.btn-warn:hover { background: var(--amber); color: #000; }

    /* Canvas Tactical Area */
    .radar-container { flex: 1; position: relative; background: var(--bg); display: flex; flex-direction: column; }
    canvas { width: 100%; height: 100%; display: block; cursor: crosshair; }

    /* Floating Zoom Controls */
    .zoom-toolbar {
      position: absolute; top: 16px; right: 16px;
      background: rgba(13, 21, 36, 0.9); border: 1px solid var(--border);
      border-radius: 8px; padding: 4px; display: flex; flex-direction: column; gap: 4px;
    }
    .zoom-btn {
      width: 32px; height: 32px; font-size: 16px; display: flex; align-items: center; justify-content: center;
      padding: 0;
    }

    /* Radar Legend Overlay */
    .radar-legend {
      position: absolute; bottom: 32px; left: 16px;
      background: rgba(13, 21, 36, 0.85); border: 1px solid var(--border);
      border-radius: 6px; padding: 8px 12px; font-size: 11px; font-family: monospace; color: var(--dim);
    }

    /* Bottom Raw Data Banner */
    .raw-bar {
      height: 26px; line-height: 26px; padding: 0 14px;
      background: #04070c; border-top: 1px solid rgba(255,255,255,0.06);
      font-size: 11px; font-family: monospace; color: #34d399;
      white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
    }
  </style>
</head>
<body>
  <header>
    <div class="brand">
      <div class="brand-badge">&#9650;</div>
      <div>
        <div class="brand-title">MARS ROVER // TACTICAL GROUND STATION</div>
        <div class="brand-sub">OFFLINE ROBOTIC NAVIGATION SUITE</div>
      </div>
    </div>
    <div class="header-right">
      <div id="utcClock" style="font-family: monospace; font-size: 12px; color: var(--cyan);">--:--:-- UTC</div>
      <div class="status-pill" id="statusBadge">
        <span class="status-dot" id="statusDot"></span>
        <span id="statusTxt">LINK ACTIVE</span>
      </div>
    </div>
  </header>

  <div class="main-view">
    <!-- Telemetry Sidebar -->
    <div class="sidebar">
      <!-- Geographic Position -->
      <div class="card">
        <div class="card-accent"></div>
        <div class="card-label">GEOGRAPHIC COORDINATES</div>
        <div class="card-val" id="coordsVal">--.------, --.------</div>
        <div class="card-meta">
          <span>LAT: <b id="latVal" style="color:#fff;">--</b></span>
          <span>LNG: <b id="lngVal" style="color:#fff;">--</b></span>
        </div>
      </div>

      <!-- Rover Relative Metric Displacement (1m/2m Precision) -->
      <div class="card">
        <div class="card-accent" style="background: #3b82f6;"></div>
        <div class="card-label">RELATIVE ODOMETRY (FROM ORIGIN)</div>
        <div class="card-val" id="dispTotal" style="color: #38bdf8;">0.00 m</div>
        <div class="card-meta">
          <span>ΔX (East): <b id="dispX" style="color:#fff;">+0.0m</b></span>
          <span>ΔY (North): <b id="dispY" style="color:#fff;">+0.0m</b></span>
        </div>
      </div>

      <!-- Precision & Satellites -->
      <div class="row-2">
        <div class="card">
          <div class="card-accent" style="background: var(--green);"></div>
          <div class="card-label">SATELLITES</div>
          <div class="card-val" id="satsVal" style="color: var(--green);">0</div>
          <div class="card-meta"><span id="fixState">Searching...</span></div>
        </div>
        <div class="card">
          <div class="card-accent" style="background: var(--amber);"></div>
          <div class="card-label">HDOP PRECISION</div>
          <div class="card-val" id="hdopVal" style="color: var(--amber);">--</div>
          <div class="card-meta"><span id="accVal">Est. Err: --</span></div>
        </div>
      </div>

      <!-- Speed & Elevation -->
      <div class="row-2">
        <div class="card">
          <div class="card-accent"></div>
          <div class="card-label">SPEED</div>
          <div class="card-val"><span id="speedKmh">0.0</span> <small style="font-size:11px;color:var(--dim);">km/h</small></div>
          <div class="card-meta"><span id="speedMs">0.00 m/s</span></div>
        </div>
        <div class="card">
          <div class="card-accent"></div>
          <div class="card-label">ALTITUDE</div>
          <div class="card-val"><span id="altVal">--</span> <small style="font-size:11px;color:var(--dim);">m</small></div>
          <div class="card-meta"><span>MSL Elevation</span></div>
        </div>
      </div>

      <!-- Path Metrics -->
      <div class="card">
        <div class="card-accent" style="background: #8b5cf6;"></div>
        <div class="card-label">MISSION TRAIL</div>
        <div class="card-val" id="trailLen">0.0 m</div>
        <div class="card-meta">
          <span>POINTS: <b id="pointCount">0</b></span>
          <span id="headingVal">HEADING: ---°</span>
        </div>
      </div>

      <!-- Control Buttons -->
      <div class="btn-row">
        <button onclick="resetOrigin()" class="btn-warn">ZERO ORIGIN (0,0)</button>
        <button onclick="clearTrail()">CLEAR TRAIL</button>
      </div>
    </div>

    <!-- Canvas Tactical Display -->
    <div class="radar-container">
      <canvas id="radarCanvas"></canvas>

      <!-- Zoom Bar -->
      <div class="zoom-toolbar">
        <button class="zoom-btn" onclick="adjustZoom(1.4)" title="Zoom In (+)">+</button>
        <button class="zoom-btn" onclick="adjustZoom(0.7)" title="Zoom Out (-)">-</button>
        <button class="zoom-btn" onclick="resetView()" title="Center Rover" style="font-size: 11px;">⊙</button>
      </div>

      <!-- Grid Scale Legend -->
      <div class="radar-legend">
        GRID RING INTERVAL: <b id="gridIntervalText" style="color: var(--cyan);">2.0 METERS</b><br>
        CENTER: ORIGIN (0m, 0m)
      </div>
    </div>
  </div>

  <div class="raw-bar" id="rawBar">
    [NMEA] Initializing RadioLink SE100 stream...
  </div>

  <script>
    // Canvas & Navigation Engine (100% Offline)
    const canvas = document.getElementById('radarCanvas');
    const ctx = canvas.getContext('2d');

    let originLat = null, originLng = null;
    let roverLat = null, roverLng = null;
    let roverX = 0, roverY = 0; // Relative meters East/North
    let trail = []; // [{x, y}]
    let totalTrailDistance = 0;

    // Viewport & Scaling
    let pixelsPerMeter = 25; // 25 px = 1 meter (default zoom allows seeing 1m clearly!)
    let panOffsetX = 0, panOffsetY = 0;

    function resizeCanvas() {
      canvas.width = canvas.parentElement.clientWidth;
      canvas.height = canvas.parentElement.clientHeight;
      drawRadar();
    }
    window.addEventListener('resize', resizeCanvas);

    function adjustZoom(factor) {
      pixelsPerMeter = Math.max(0.5, Math.min(200, pixelsPerMeter * factor));
      updateGridLegend();
      drawRadar();
    }

    function resetView() {
      panOffsetX = -roverX * pixelsPerMeter;
      panOffsetY = roverY * pixelsPerMeter;
      drawRadar();
    }

    function resetOrigin() {
      if (roverLat !== null) {
        originLat = roverLat;
        originLng = roverLng;
        trail = [];
        totalTrailDistance = 0;
        roverX = 0;
        roverY = 0;
        panOffsetX = 0;
        panOffsetY = 0;
        document.getElementById('dispTotal').innerText = '0.00 m';
        document.getElementById('dispX').innerText = '+0.0m';
        document.getElementById('dispY').innerText = '+0.0m';
        document.getElementById('trailLen').innerText = '0.0 m';
        document.getElementById('pointCount').innerText = '0';
        drawRadar();
      }
    }

    function clearTrail() {
      trail = [];
      totalTrailDistance = 0;
      document.getElementById('trailLen').innerText = '0.0 m';
      document.getElementById('pointCount').innerText = '0';
      drawRadar();
    }

    function updateGridLegend() {
      let interval = 1;
      if (pixelsPerMeter < 2) interval = 50;
      else if (pixelsPerMeter < 6) interval = 20;
      else if (pixelsPerMeter < 15) interval = 10;
      else if (pixelsPerMeter < 35) interval = 2;
      else interval = 1;
      document.getElementById('gridIntervalText').innerText = `${interval.toFixed(1)} METERS`;
    }

    // Convert GPS Lat/Lng to metric displacement (Meters North/East)
    function latLngToMeters(lat, lng) {
      const R = 6378137.0; // Earth radius in meters
      const dLat = (lat - originLat) * (Math.PI / 180.0);
      const dLng = (lng - originLng) * (Math.PI / 180.0);
      const y = dLat * R; // North is +Y
      const x = dLng * R * Math.cos(((originLat + lat) / 2) * (Math.PI / 180.0)); // East is +X
      return { x, y };
    }

    function drawRadar() {
      const w = canvas.width;
      const h = canvas.height;
      if (!w || !h) return;

      // Dark Tactical Background
      ctx.fillStyle = '#070b12';
      ctx.fillRect(0, 0, w, h);

      const cx = w / 2 + panOffsetX;
      const cy = h / 2 + panOffsetY;

      // Determine ring interval based on zoom
      let stepMeters = 1;
      if (pixelsPerMeter < 2) stepMeters = 50;
      else if (pixelsPerMeter < 6) stepMeters = 20;
      else if (pixelsPerMeter < 15) stepMeters = 10;
      else if (pixelsPerMeter < 35) stepMeters = 2;
      else stepMeters = 1;

      const ringStepPx = stepMeters * pixelsPerMeter;

      // Draw Grid Circles
      ctx.lineWidth = 1;
      ctx.font = '10px monospace';
      ctx.fillStyle = '#475569';
      ctx.textAlign = 'left';

      for (let r = ringStepPx; r < Math.max(w, h); r += ringStepPx) {
        ctx.strokeStyle = (r % (ringStepPx * 5) === 0) ? 'rgba(56, 189, 248, 0.18)' : 'rgba(255, 255, 255, 0.05)';
        ctx.beginPath();
        ctx.arc(cx, cy, r, 0, Math.PI * 2);
        ctx.stroke();

        const metersVal = (r / pixelsPerMeter).toFixed(0);
        ctx.fillText(`${metersVal}m`, cx + r + 3, cy - 3);
      }

      // Crosshairs (North / South, East / West axes)
      ctx.strokeStyle = 'rgba(56, 189, 248, 0.22)';
      ctx.beginPath();
      ctx.moveTo(cx, 0); ctx.lineTo(cx, h);
      ctx.moveTo(0, cy); ctx.lineTo(w, cy);
      ctx.stroke();

      // Cardinal Labels (N, S, E, W)
      ctx.font = 'bold 12px monospace';
      ctx.fillStyle = '#00f0ff';
      ctx.textAlign = 'center';
      ctx.fillText('N (NORTH)', cx, 24);
      ctx.fillText('S', cx, h - 12);
      ctx.textAlign = 'right';
      ctx.fillText('W', 24, cy + 4);
      ctx.textAlign = 'left';
      ctx.fillText('E', w - 24, cy + 4);

      // Draw Origin Marker (0,0 Base Station)
      ctx.strokeStyle = '#f59e0b';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(cx, cy, 6, 0, Math.PI * 2);
      ctx.stroke();
      ctx.fillStyle = '#f59e0b';
      ctx.font = '10px monospace';
      ctx.fillText('ORIGIN (0m)', cx + 10, cy + 14);

      // Draw Breadcrumb Trail
      if (trail.length > 1) {
        ctx.strokeStyle = '#00f0ff';
        ctx.lineWidth = 2.5;
        ctx.setLineDash([4, 4]);
        ctx.beginPath();
        for (let i = 0; i < trail.length; i++) {
          const px = cx + trail[i].x * pixelsPerMeter;
          const py = cy - trail[i].y * pixelsPerMeter; // Canvas Y is inverted
          if (i === 0) ctx.moveTo(px, py);
          else ctx.lineTo(px, py);
        }
        ctx.stroke();
        ctx.setLineDash([]);
      }

      // Draw Rover Position
      const roverPx = cx + roverX * pixelsPerMeter;
      const roverPy = cy - roverY * pixelsPerMeter;

      // Glow Ring
      const grad = ctx.createRadialGradient(roverPx, roverPy, 4, roverPx, roverPy, 22);
      grad.addColorStop(0, 'rgba(0, 240, 255, 0.8)');
      grad.addColorStop(1, 'rgba(0, 240, 255, 0)');
      ctx.fillStyle = grad;
      ctx.beginPath();
      ctx.arc(roverPx, roverPy, 22, 0, Math.PI * 2);
      ctx.fill();

      // Rover Core
      ctx.fillStyle = '#00f0ff';
      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(roverPx, roverPy, 7, 0, Math.PI * 2);
      ctx.fill();
      ctx.stroke();

      // Rover Label
      ctx.fillStyle = '#ffffff';
      ctx.font = 'bold 11px monospace';
      ctx.textAlign = 'left';
      ctx.fillText(`ROVER [${roverX.toFixed(1)}m, ${roverY.toFixed(1)}m]`, roverPx + 12, roverPy - 8);
    }

    // Telemetry Update Loop
    async function fetchTelemetry() {
      try {
        const res = await fetch('/api/telemetry');
        const d = await res.json();

        // Update Bottom Raw Bar
        document.getElementById('rawBar').innerText = `[NMEA] ${d.raw || 'Tracking...'} | Bytes Processed: ${d.chars}`;

        if (d.fix && d.lat !== 0) {
          roverLat = d.lat;
          roverLng = d.lng;

          // Set Origin on first fix
          if (originLat === null) {
            originLat = d.lat;
            originLng = d.lng;
          }

          // Calculate displacement in meters
          const pos = latLngToMeters(d.lat, d.lng);
          roverX = pos.x;
          roverY = pos.y;

          const totalDisp = Math.sqrt(roverX * roverX + roverY * roverY);

          // Update Trail if moved > 0.4m
          if (trail.length === 0) {
            trail.push({ x: roverX, y: roverY });
          } else {
            const last = trail[trail.length - 1];
            const seg = Math.sqrt((roverX - last.x)**2 + (roverY - last.y)**2);
            if (seg >= 0.4) {
              trail.push({ x: roverX, y: roverY });
              totalTrailDistance += seg;
              document.getElementById('trailLen').innerText = `${totalTrailDistance.toFixed(1)} m`;
              document.getElementById('pointCount').innerText = trail.length;
            }
          }

          // Update UI
          document.getElementById('coordsVal').innerText = `${d.lat.toFixed(6)}°, ${d.lng.toFixed(6)}°`;
          document.getElementById('latVal').innerText = `${d.lat.toFixed(6)}°`;
          document.getElementById('lngVal').innerText = `${d.lng.toFixed(6)}°`;

          document.getElementById('dispTotal').innerText = `${totalDisp.toFixed(2)} m`;
          document.getElementById('dispX').innerText = `${roverX >= 0 ? '+' : ''}${roverX.toFixed(1)}m`;
          document.getElementById('dispY').innerText = `${roverY >= 0 ? '+' : ''}${roverY.toFixed(1)}m`;

          document.getElementById('satsVal').innerText = d.satellites;
          document.getElementById('fixState').innerText = `3D FIX (${d.satellites} Sats)`;

          document.getElementById('hdopVal').innerText = d.hdop.toFixed(2);
          const estErr = (d.hdop * 2.5).toFixed(1);
          document.getElementById('accVal').innerText = `Est. Err: ±${estErr}m`;

          document.getElementById('speedKmh').innerText = d.speed.toFixed(1);
          document.getElementById('speedMs').innerText = `${(d.speed / 3.6).toFixed(2)} m/s`;
          document.getElementById('altVal').innerText = d.altitude.toFixed(1);

          if (d.time) document.getElementById('utcClock').innerText = `${d.time} UTC`;

          document.getElementById('statusTxt').innerText = `3D FIX ACQUIRED`;
          document.getElementById('statusBadge').style.borderColor = '#10b981';
          document.getElementById('statusBadge').style.color = '#10b981';
          document.getElementById('statusDot').style.background = '#10b981';

          drawRadar();
        } else {
          document.getElementById('satsVal').innerText = d.satellites;
          document.getElementById('fixState').innerText = `Acquiring (${d.satellites} Sats)...`;
          document.getElementById('statusTxt').innerText = `SEARCHING SATS (${d.satellites})`;
          document.getElementById('statusBadge').style.borderColor = '#f59e0b';
          document.getElementById('statusBadge').style.color = '#f59e0b';
          document.getElementById('statusDot').style.background = '#f59e0b';
        }
      } catch (e) {
        document.getElementById('statusTxt').innerText = `DISCONNECTED`;
        document.getElementById('statusBadge').style.borderColor = '#ef4444';
        document.getElementById('statusBadge').style.color = '#ef4444';
        document.getElementById('statusDot').style.background = '#ef4444';
      }
    }

    // Drag to Pan Canvas
    let isDragging = false;
    let dragStartX = 0, dragStartY = 0;
    canvas.addEventListener('mousedown', (e) => {
      isDragging = true;
      dragStartX = e.clientX - panOffsetX;
      dragStartY = e.clientY - panOffsetY;
    });
    window.addEventListener('mousemove', (e) => {
      if (!isDragging) return;
      panOffsetX = e.clientX - dragStartX;
      panOffsetY = e.clientY - dragStartY;
      drawRadar();
    });
    window.addEventListener('mouseup', () => { isDragging = false; });

    // Touch support for mobile/tablets
    canvas.addEventListener('touchstart', (e) => {
      if (e.touches.length === 1) {
        isDragging = true;
        dragStartX = e.touches[0].clientX - panOffsetX;
        dragStartY = e.touches[0].clientY - panOffsetY;
      }
    });
    window.addEventListener('touchmove', (e) => {
      if (!isDragging || e.touches.length !== 1) return;
      panOffsetX = e.touches[0].clientX - dragStartX;
      panOffsetY = e.touches[0].clientY - dragStartY;
      drawRadar();
    });
    window.addEventListener('touchend', () => { isDragging = false; });

    // Wheel to Zoom
    canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      adjustZoom(e.deltaY < 0 ? 1.2 : 0.83);
    });

    resizeCanvas();
    updateGridLegend();
    setInterval(fetchTelemetry, 1000);
  </script>
</body>
</html>
)rawliteral";

// API Endpoint: /api/telemetry
void handleTelemetry() {
  char json[384];
  char timeStr[16] = "--:--:--";
  char dateStr[16] = "----/--/--";

  if (gps.time.isValid()) {
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
  }
  if (gps.date.isValid()) {
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", gps.date.year(), gps.date.month(), gps.date.day());
  }

  snprintf(json, sizeof(json),
    "{"
      "\"fix\":%s,"
      "\"lat\":%.6f,"
      "\"lng\":%.6f,"
      "\"satellites\":%u,"
      "\"hdop\":%.2f,"
      "\"altitude\":%.1f,"
      "\"speed\":%.1f,"
      "\"time\":\"%s\","
      "\"date\":\"%s\","
      "\"chars\":%u,"
      "\"raw\":\"%s\""
    "}",
    gps.location.isValid() ? "true" : "false",
    gps.location.isValid() ? gps.location.lat() : 0.0,
    gps.location.isValid() ? gps.location.lng() : 0.0,
    gps.satellites.isValid() ? gps.satellites.value() : 0,
    gps.hdop.isValid() ? gps.hdop.hdop() : 99.9,
    gps.altitude.isValid() ? gps.altitude.meters() : 0.0,
    gps.speed.isValid() ? gps.speed.kmph() : 0.0,
    timeStr,
    dateStr,
    totalChars,
    lastRawNMEA
  );

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F("  MARS ROVER // TACTICAL RADAR GROUND STATION     "));
  Serial.println(F("=================================================="));

  // Start GPS on UART2 (Pins 16 & 17) @ 38400 baud
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.printf("[OK] GPS Serial listening on Pin %d @ %u baud\n", GPS_RX_PIN, GPS_BAUD);

  // Start Wi-Fi Access Point (Stand-alone, 0 internet needed)
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();

  Serial.printf("[OK] Wi-Fi Ground Station AP Active!\n");
  Serial.printf("     SSID:     %s\n", AP_SSID);
  Serial.printf("     Password: %s\n", AP_PASS);
  Serial.printf("     Open URL: http://%s\n\n", apIP.toString().c_str());

  // Web routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/telemetry", HTTP_GET, handleTelemetry);
  server.begin();
  Serial.println(F("[OK] Telemetry Server active on port 80"));
}

void loop() {
  server.handleClient();

  static char lineBuf[128];
  static int lineIdx = 0;

  while (GPSSerial.available() > 0) {
    char c = GPSSerial.read();
    totalChars++;
    gps.encode(c);

    // Save latest NMEA sentence for web display
    if (c == '\n' || c == '\r') {
      if (lineIdx > 5 && lineBuf[0] == '$') {
        lineBuf[lineIdx] = '\0';
        strncpy(lastRawNMEA, lineBuf, sizeof(lastRawNMEA) - 1);
        lastRawNMEA[sizeof(lastRawNMEA) - 1] = '\0';
      }
      lineIdx = 0;
    } else if (lineIdx < (int)sizeof(lineBuf) - 1) {
      lineBuf[lineIdx++] = c;
    }
  }

  // Serial debug output every 3s
  if (millis() - lastSerialPrint >= 3000) {
    lastSerialPrint = millis();

    if (gps.location.isValid()) {
      Serial.printf("[3D FIX] Lat: %.6f, Lng: %.6f | Sats: %u | HDOP: %.2f | Alt: %.1fm\n",
                    gps.location.lat(), gps.location.lng(),
                    gps.satellites.value(), gps.hdop.hdop(),
                    gps.altitude.meters());
    } else {
      Serial.printf("[WAITING FIX] Sats in view: %u | Bytes: %u\n",
                    gps.satellites.value(), totalChars);
    }
  }
}
