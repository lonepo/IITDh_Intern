/**
 * PLL Blog — Canvas 2D Animations
 * Four self-contained animation classes:
 *   1. GridWaveform     — 3-phase sinusoids at 120° apart
 *   2. SPWMWaveform     — 3-phase SPWM out of phase with grid
 *   3. ClarkeTransform  — ABC → αβ with rotating vector
 *   4. ParkPLL          — dq frame lock-on with damping slider
 */

/* ── Shared Helpers ── */
function getThemeColors() {
  const style = getComputedStyle(document.documentElement);
  return {
    phaseA: style.getPropertyValue('--accent-blue').trim() || '#4da6ff',
    phaseB: style.getPropertyValue('--accent-green').trim() || '#3ddc84',
    phaseC: style.getPropertyValue('--accent-orange').trim() || '#ff8c42',
    text: style.getPropertyValue('--text-muted').trim() || '#3f5e4a',
    textPrimary: style.getPropertyValue('--text-primary').trim() || '#e2f0e8',
    bg: style.getPropertyValue('--bg-secondary').trim() || '#0f1a12',
    grid: 'rgba(255,255,255,0.05)',
    axisFaint: 'rgba(255,255,255,0.12)',
  };
}

function setupCanvas(canvas) {
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;
  const ctx = canvas.getContext('2d');
  ctx.scale(dpr, dpr);
  return { ctx, w: rect.width, h: rect.height };
}

function drawGrid(ctx, w, h, cols, rows) {
  const c = getThemeColors();
  ctx.strokeStyle = c.grid;
  ctx.lineWidth = 0.5;
  for (let i = 1; i < cols; i++) {
    const x = (w / cols) * i;
    ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
  }
  for (let j = 1; j < rows; j++) {
    const y = (h / rows) * j;
    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
  }
}

function drawAxis(ctx, w, h, labelX, labelY) {
  const c = getThemeColors();
  // Horizontal center axis
  ctx.strokeStyle = c.axisFaint;
  ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2); ctx.stroke();

  // Labels
  ctx.font = '11px JetBrains Mono, monospace';
  ctx.fillStyle = c.text;
  if (labelX) ctx.fillText(labelX, w - 30, h / 2 + 16);
  if (labelY) ctx.fillText(labelY, 8, 16);
}

function glowLine(ctx, points, color, lineWidth = 2.5) {
  if (points.length < 2) return;
  // Glow pass
  ctx.save();
  ctx.shadowBlur = 10;
  ctx.shadowColor = color + '66';
  ctx.strokeStyle = color;
  ctx.lineWidth = lineWidth;
  ctx.lineJoin = 'round';
  ctx.beginPath();
  ctx.moveTo(points[0][0], points[0][1]);
  for (let i = 1; i < points.length; i++) ctx.lineTo(points[i][0], points[i][1]);
  ctx.stroke();
  ctx.restore();

  // Crisp pass
  ctx.strokeStyle = color;
  ctx.lineWidth = lineWidth * 0.7;
  ctx.lineJoin = 'round';
  ctx.beginPath();
  ctx.moveTo(points[0][0], points[0][1]);
  for (let i = 1; i < points.length; i++) ctx.lineTo(points[i][0], points[i][1]);
  ctx.stroke();
}


/* ═══════════════════════════════════════════════
   1. GridWaveform — Three-phase sinusoids
═══════════════════════════════════════════════ */
class GridWaveform {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.running = false;
    this.t = 0;
    this.rafId = null;
  }

  init() {
    this.resize();
    this.draw();
  }

  resize() {
    const r = setupCanvas(this.canvas);
    this.ctx = r.ctx;
    this.w = r.w;
    this.h = r.h;
  }

  start() {
    if (this.running) return;
    this.running = true;
    this.loop();
  }

  stop() {
    this.running = false;
    if (this.rafId) cancelAnimationFrame(this.rafId);
  }

  loop() {
    if (!this.running) return;
    this.t += 0.025;
    this.draw();
    this.rafId = requestAnimationFrame(() => this.loop());
  }

  draw() {
    const { ctx, w, h, t } = this;
    const c = getThemeColors();
    const amp = h * 0.35;
    const cy = h / 2;
    const freq = 2; // cycles visible on screen

    ctx.clearRect(0, 0, w, h);
    drawGrid(ctx, w, h, 12, 6);
    drawAxis(ctx, w, h, 't', 'v');

    const phases = [
      { offset: 0, color: c.phaseA },
      { offset: (-2 * Math.PI) / 3, color: c.phaseB },
      { offset: (2 * Math.PI) / 3, color: c.phaseC },
    ];

    for (const ph of phases) {
      const pts = [];
      for (let px = 0; px < w; px += 2) {
        const x = (px / w) * freq * 2 * Math.PI;
        const y = cy - amp * Math.sin(x + t + ph.offset);
        pts.push([px, y]);
      }
      glowLine(ctx, pts, ph.color);
    }
  }
}


/* ═══════════════════════════════════════════════
   2. SPWMWaveform — Out-of-phase inverter output
═══════════════════════════════════════════════ */
class SPWMWaveform {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.running = false;
    this.t = 0;
    this.rafId = null;
    this.phaseShift = Math.PI / 4; // deliberate mismatch
  }

  init() { this.resize(); this.draw(); }

  resize() {
    const r = setupCanvas(this.canvas);
    this.ctx = r.ctx; this.w = r.w; this.h = r.h;
  }

  start() { if (this.running) return; this.running = true; this.loop(); }
  stop() { this.running = false; if (this.rafId) cancelAnimationFrame(this.rafId); }

  loop() {
    if (!this.running) return;
    this.t += 0.025;
    this.draw();
    this.rafId = requestAnimationFrame(() => this.loop());
  }

  draw() {
    const { ctx, w, h, t, phaseShift } = this;
    const c = getThemeColors();
    const amp = h * 0.32;
    const cy = h / 2;
    const freq = 2;

    ctx.clearRect(0, 0, w, h);
    drawGrid(ctx, w, h, 12, 6);
    drawAxis(ctx, w, h, 't', 'v');

    // Grid reference (faded)
    const gridPhases = [0, (-2 * Math.PI) / 3, (2 * Math.PI) / 3];
    const gridColors = [c.phaseA, c.phaseB, c.phaseC];

    for (let i = 0; i < 3; i++) {
      const pts = [];
      for (let px = 0; px < w; px += 2) {
        const x = (px / w) * freq * 2 * Math.PI;
        const y = cy - amp * Math.sin(x + t + gridPhases[i]);
        pts.push([px, y]);
      }
      // Faded grid reference
      ctx.globalAlpha = 0.18;
      glowLine(ctx, pts, gridColors[i], 1.5);
      ctx.globalAlpha = 1;
    }

    // Inverter SPWM (vivid, phase-shifted)
    for (let i = 0; i < 3; i++) {
      const pts = [];
      for (let px = 0; px < w; px += 2) {
        const x = (px / w) * freq * 2 * Math.PI;
        const y = cy - amp * Math.sin(x + t + gridPhases[i] + phaseShift);
        pts.push([px, y]);
      }
      glowLine(ctx, pts, gridColors[i]);
    }

    // Phase shift indicator
    ctx.font = '11px JetBrains Mono, monospace';
    ctx.fillStyle = c.text;
    ctx.fillText(`Δφ = ${((phaseShift * 180) / Math.PI).toFixed(0)}°`, w - 70, 20);
  }
}


/* ═══════════════════════════════════════════════
   3. ClarkeTransform — ABC → αβ plane
═══════════════════════════════════════════════ */
class ClarkeTransform {
  constructor(abcCanvasId, abCanvasId) {
    this.abcCanvas = document.getElementById(abcCanvasId);
    this.abCanvas = document.getElementById(abCanvasId);
    this.running = false;
    this.t = 0;
    this.rafId = null;
    this.trail = []; // trailing dots for αβ
  }

  init() { this.resize(); this.draw(); }

  resize() {
    const r1 = setupCanvas(this.abcCanvas);
    this.ctxAbc = r1.ctx; this.w1 = r1.w; this.h1 = r1.h;
    const r2 = setupCanvas(this.abCanvas);
    this.ctxAb = r2.ctx; this.w2 = r2.w; this.h2 = r2.h;
  }

  start() { if (this.running) return; this.running = true; this.loop(); }
  stop() { this.running = false; if (this.rafId) cancelAnimationFrame(this.rafId); }

  loop() {
    if (!this.running) return;
    this.t += 0.012;
    this.draw();
    this.rafId = requestAnimationFrame(() => this.loop());
  }

  draw() {
    const { ctxAbc, w1, h1, ctxAb, w2, h2, t } = this;
    const c = getThemeColors();

    // ─── Left: ABC waveforms ───
    ctxAbc.clearRect(0, 0, w1, h1);
    drawGrid(ctxAbc, w1, h1, 8, 4);
    drawAxis(ctxAbc, w1, h1, 't', '');

    const amp = h1 * 0.35;
    const cy = h1 / 2;
    const freq = 1.5;
    const phases = [0, (-2 * Math.PI) / 3, (2 * Math.PI) / 3];
    const colors = [c.phaseA, c.phaseB, c.phaseC];

    for (let i = 0; i < 3; i++) {
      const pts = [];
      for (let px = 0; px < w1; px += 2) {
        const x = (px / w1) * freq * 2 * Math.PI;
        const y = cy - amp * Math.sin(x + t + phases[i]);
        pts.push([px, y]);
      }
      glowLine(ctxAbc, pts, colors[i]);
    }

    // ─── Right: αβ plane ───
    ctxAb.clearRect(0, 0, w2, h2);
    const cx2 = w2 / 2;
    const cy2 = h2 / 2;
    const radius = Math.min(w2, h2) * 0.35;

    // Grid circle
    ctxAb.strokeStyle = c.grid;
    ctxAb.lineWidth = 1;
    ctxAb.beginPath();
    ctxAb.arc(cx2, cy2, radius, 0, 2 * Math.PI);
    ctxAb.stroke();

    // Axes
    ctxAb.strokeStyle = c.axisFaint;
    ctxAb.lineWidth = 0.8;
    ctxAb.beginPath(); ctxAb.moveTo(0, cy2); ctxAb.lineTo(w2, cy2); ctxAb.stroke();
    ctxAb.beginPath(); ctxAb.moveTo(cx2, 0); ctxAb.lineTo(cx2, h2); ctxAb.stroke();

    // Axis labels
    ctxAb.font = '11px JetBrains Mono, monospace';
    ctxAb.fillStyle = c.text;
    ctxAb.fillText('α', w2 - 16, cy2 - 6);
    ctxAb.fillText('β', cx2 + 6, 14);

    // Clarke transform
    const va = Math.sin(t);
    const vb = Math.sin(t - (2 * Math.PI) / 3);
    const vc = Math.sin(t + (2 * Math.PI) / 3);

    const vAlpha = va - 0.5 * vb - 0.5 * vc;
    const vBeta = (Math.sqrt(3) / 2) * (vb - vc);

    const px = cx2 + vAlpha * radius;
    const py = cy2 - vBeta * radius;

    // Trail
    this.trail.push({ x: px, y: py, age: 0 });
    if (this.trail.length > 120) this.trail.shift();
    for (const dot of this.trail) {
      dot.age += 0.01;
      const alpha = Math.max(0, 1 - dot.age);
      ctxAb.fillStyle = `rgba(77, 166, 255, ${alpha * 0.5})`;
      ctxAb.beginPath();
      ctxAb.arc(dot.x, dot.y, 1.5, 0, 2 * Math.PI);
      ctxAb.fill();
    }

    // Vector line
    ctxAb.save();
    ctxAb.shadowBlur = 12;
    ctxAb.shadowColor = c.phaseA + '88';
    ctxAb.strokeStyle = c.phaseA;
    ctxAb.lineWidth = 2.5;
    ctxAb.beginPath();
    ctxAb.moveTo(cx2, cy2);
    ctxAb.lineTo(px, py);
    ctxAb.stroke();
    ctxAb.restore();

    // Vector tip dot
    ctxAb.fillStyle = c.phaseA;
    ctxAb.shadowBlur = 8;
    ctxAb.shadowColor = c.phaseA + 'aa';
    ctxAb.beginPath();
    ctxAb.arc(px, py, 5, 0, 2 * Math.PI);
    ctxAb.fill();
    ctxAb.shadowBlur = 0;
  }
}


/* ═══════════════════════════════════════════════
   4. ParkPLL — dq frame + PLL lock-on animation
═══════════════════════════════════════════════ */
class ParkPLL {
  constructor(pllCanvasId, errorCanvasId) {
    this.pllCanvas = document.getElementById(pllCanvasId);
    this.errorCanvas = document.getElementById(errorCanvasId);
    this.running = false;
    this.pllStarted = false;
    this.t = 0;
    this.rafId = null;

    // PLL state
    this.thetaGrid = 0;       // actual grid angle
    this.thetaHat = 0;        // estimated angle
    this.omegaHat = 0;        // estimated frequency
    this.integral = 0;        // PI integrator
    this.omegaGrid = 2 * Math.PI * 0.4; // visual angular speed (~0.4 rev/sec, slow enough to follow)

    // Damping
    this.zeta = 0.707;
    this.omegaN = 12;         // natural frequency for visual convergence speed

    // Error history for plot
    this.errorHistory = [];
    this.maxHistory = 200;
  }

  init() {
    this.resize();
    this.draw();
  }

  resize() {
    const r1 = setupCanvas(this.pllCanvas);
    this.ctxPll = r1.ctx; this.w1 = r1.w; this.h1 = r1.h;
    const r2 = setupCanvas(this.errorCanvas);
    this.ctxErr = r2.ctx; this.w2 = r2.w; this.h2 = r2.h;
  }

  start() { if (this.running) return; this.running = true; this.loop(); }
  stop() { this.running = false; if (this.rafId) cancelAnimationFrame(this.rafId); }

  startPLL() {
    this.pllStarted = true;
    this.thetaHat = this.thetaGrid + Math.PI * 0.8; // start with offset
    this.omegaHat = this.omegaGrid * 0.5; // start slow
    this.integral = 0;
    this.errorHistory = [];
  }

  resetPLL() {
    this.pllStarted = false;
    this.thetaHat = 0;
    this.omegaHat = 0;
    this.integral = 0;
    this.errorHistory = [];
  }

  setDamping(z) {
    this.zeta = z;
  }

  loop() {
    if (!this.running) return;
    const dt = 1 / 60;
    this.t += dt;

    // Grid angle advances
    this.thetaGrid += this.omegaGrid * dt;

    // PLL tracking
    if (this.pllStarted) {
      // Phase error via vq (simplified: sin of angle difference)
      const thetaError = Math.sin(this.thetaGrid - this.thetaHat);

      // PI controller gains derived from damping
      const Kp = 2 * this.zeta * this.omegaN;
      const Ki = this.omegaN * this.omegaN;

      this.integral += thetaError * dt;
      const omegaCorrection = Kp * thetaError + Ki * this.integral;
      this.omegaHat = this.omegaGrid + omegaCorrection;

      // Advance estimated angle
      this.thetaHat += this.omegaHat * dt;

      // Track error
      let errorDeg = ((this.thetaGrid - this.thetaHat) * 180) / Math.PI;
      // Wrap to [-180, 180]
      errorDeg = ((errorDeg + 180) % 360 + 360) % 360 - 180;
      this.errorHistory.push(errorDeg);
      if (this.errorHistory.length > this.maxHistory) this.errorHistory.shift();
    }

    this.draw();
    this.rafId = requestAnimationFrame(() => this.loop());
  }

  draw() {
    this.drawPLLPlane();
    this.drawErrorPlot();
  }

  drawPLLPlane() {
    const { ctxPll: ctx, w1: w, h1: h, thetaGrid, thetaHat, pllStarted } = this;
    const c = getThemeColors();

    ctx.clearRect(0, 0, w, h);

    const cx = w / 2;
    const cy = h / 2;
    const radius = Math.min(w, h) * 0.35;

    // Background circle
    ctx.strokeStyle = c.grid;
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, 2 * Math.PI);
    ctx.stroke();

    // Static axes
    ctx.strokeStyle = c.axisFaint;
    ctx.lineWidth = 0.8;
    ctx.beginPath(); ctx.moveTo(cx - radius - 20, cy); ctx.lineTo(cx + radius + 20, cy); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(cx, cy - radius - 20); ctx.lineTo(cx, cy + radius + 20); ctx.stroke();

    ctx.font = '11px JetBrains Mono, monospace';
    ctx.fillStyle = c.text;
    ctx.fillText('α', cx + radius + 8, cy - 4);
    ctx.fillText('β', cx + 6, cy - radius - 8);

    // Grid vector (αβ)
    const gx = cx + radius * Math.cos(thetaGrid);
    const gy = cy - radius * Math.sin(thetaGrid);

    ctx.save();
    ctx.shadowBlur = 12;
    ctx.shadowColor = c.phaseA + '88';
    ctx.strokeStyle = c.phaseA;
    ctx.lineWidth = 3;
    ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(gx, gy); ctx.stroke();
    ctx.restore();

    // Grid vector tip
    ctx.fillStyle = c.phaseA;
    ctx.beginPath(); ctx.arc(gx, gy, 6, 0, 2 * Math.PI); ctx.fill();

    // Label
    ctx.fillStyle = c.phaseA;
    ctx.font = 'bold 12px JetBrains Mono, monospace';
    const labelOffset = 18;
    ctx.fillText('grid', gx + labelOffset * Math.cos(thetaGrid), gy - labelOffset * Math.sin(thetaGrid));

    // dq frame (if PLL started)
    if (pllStarted) {
      const dLen = radius * 0.9;

      // d-axis
      const dx = cx + dLen * Math.cos(thetaHat);
      const dy = cy - dLen * Math.sin(thetaHat);
      ctx.save();
      ctx.setLineDash([6, 4]);
      ctx.strokeStyle = c.phaseB;
      ctx.lineWidth = 2;
      ctx.shadowBlur = 8;
      ctx.shadowColor = c.phaseB + '66';
      ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(dx, dy); ctx.stroke();
      ctx.restore();

      // q-axis (90° ahead)
      const qAngle = thetaHat + Math.PI / 2;
      const qx = cx + dLen * Math.cos(qAngle);
      const qy = cy - dLen * Math.sin(qAngle);
      ctx.save();
      ctx.setLineDash([6, 4]);
      ctx.strokeStyle = c.phaseC;
      ctx.lineWidth = 2;
      ctx.shadowBlur = 8;
      ctx.shadowColor = c.phaseC + '66';
      ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(qx, qy); ctx.stroke();
      ctx.restore();

      // Labels
      ctx.font = 'bold 11px JetBrains Mono, monospace';
      ctx.fillStyle = c.phaseB;
      ctx.fillText('d', dx + 10 * Math.cos(thetaHat), dy - 10 * Math.sin(thetaHat));
      ctx.fillStyle = c.phaseC;
      ctx.fillText('q', qx + 10 * Math.cos(qAngle), qy - 10 * Math.sin(qAngle));

      // Arc showing angle error
      let errorAngle = thetaGrid - thetaHat;
      // Normalize
      errorAngle = Math.atan2(Math.sin(errorAngle), Math.cos(errorAngle));

      if (Math.abs(errorAngle) > 0.02) {
        ctx.strokeStyle = '#ff4d6a88';
        ctx.lineWidth = 1.5;
        ctx.setLineDash([]);
        ctx.beginPath();
        const arcRadius = radius * 0.3;
        if (errorAngle > 0) {
          ctx.arc(cx, cy, arcRadius, -thetaHat, -thetaGrid, true);
        } else {
          ctx.arc(cx, cy, arcRadius, -thetaHat, -thetaGrid, false);
        }
        ctx.stroke();

        // Error label
        ctx.font = '10px JetBrains Mono, monospace';
        ctx.fillStyle = '#ff4d6a';
        const midAngle = thetaHat + errorAngle / 2;
        ctx.fillText(
          `θₑ=${((errorAngle * 180) / Math.PI).toFixed(1)}°`,
          cx + (arcRadius + 14) * Math.cos(midAngle),
          cy - (arcRadius + 14) * Math.sin(midAngle)
        );
      }
    }
  }

  drawErrorPlot() {
    const { ctxErr: ctx, w2: w, h2: h, errorHistory, pllStarted } = this;
    const c = getThemeColors();

    ctx.clearRect(0, 0, w, h);
    drawGrid(ctx, w, h, 8, 4);

    // Axes
    ctx.strokeStyle = c.axisFaint;
    ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2); ctx.stroke();

    ctx.font = '10px JetBrains Mono, monospace';
    ctx.fillStyle = c.text;
    ctx.fillText('θ error (°)', 8, 14);
    ctx.fillText('time →', w - 50, h / 2 + 14);
    ctx.fillText('0°', 4, h / 2 - 4);

    if (!pllStarted || errorHistory.length < 2) {
      ctx.fillStyle = c.text;
      ctx.font = '12px JetBrains Mono, monospace';
      ctx.textAlign = 'center';
      ctx.fillText('Start PLL to see error convergence', w / 2, h / 2);
      ctx.textAlign = 'start';
      return;
    }

    // Plot error over time
    const maxErr = 180;
    const pts = [];
    for (let i = 0; i < errorHistory.length; i++) {
      const x = (i / this.maxHistory) * w;
      const y = h / 2 - (errorHistory[i] / maxErr) * (h * 0.4);
      pts.push([x, y]);
    }

    glowLine(ctx, pts, '#ff4d6a', 2);

    // Current value readout
    const current = errorHistory[errorHistory.length - 1];
    ctx.fillStyle = '#ff4d6a';
    ctx.font = 'bold 11px JetBrains Mono, monospace';
    ctx.fillText(`${current.toFixed(1)}°`, pts[pts.length - 1][0] + 6, pts[pts.length - 1][1] - 6);
  }
}


/* ── Export ── */
window.PLLAnimations = { GridWaveform, SPWMWaveform, ClarkeTransform, ParkPLL };
