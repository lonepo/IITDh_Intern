/**
 * PLL Animations — Standalone Orchestrator
 * No ScrollTrigger — all animations auto-start on load.
 */

document.addEventListener('DOMContentLoaded', () => {

  /* ── Theme Toggle ── */
  document.getElementById('themeToggle')?.addEventListener('click', () => {
    const h = document.documentElement;
    const next = h.getAttribute('data-theme') === 'dark' ? 'light' : 'dark';
    h.setAttribute('data-theme', next);
    localStorage.setItem('theme', next);
  });

  /* ═══════════════════════════════════════════
     Animation Instances — auto-start all
  ═══════════════════════════════════════════ */
  const { GridWaveform, SPWMWaveform, ClarkeTransform, ParkPLL } = window.PLLAnimations;

  // 1. Grid Waveform
  const gridAnim = new GridWaveform('canvasGrid');
  gridAnim.init();
  gridAnim.start();

  // 2. SPWM Waveform
  const spwmAnim = new SPWMWaveform('canvasSPWM');
  spwmAnim.init();
  spwmAnim.start();

  // 3. Clarke Transform
  const clarkeAnim = new ClarkeTransform('canvasClarkeABC', 'canvasClarkeAB');
  clarkeAnim.init();
  clarkeAnim.start();

  // 4. Park PLL
  const parkAnim = new ParkPLL('canvasPLL', 'canvasError');
  parkAnim.init();
  parkAnim.start();

  // PLL button
  const pllBtn = document.getElementById('pllStartBtn');
  if (pllBtn) {
    pllBtn.addEventListener('click', () => {
      if (parkAnim.pllStarted) {
        parkAnim.resetPLL();
        pllBtn.textContent = 'Start PLL';
        pllBtn.classList.remove('running');
      } else {
        parkAnim.startPLL();
        pllBtn.textContent = 'Reset PLL';
        pllBtn.classList.add('running');
      }
    });
  }

  // Damping slider
  const dampSlider = document.getElementById('dampingSlider');
  const dampValue = document.getElementById('dampingValue');
  if (dampSlider) {
    dampSlider.addEventListener('input', () => {
      const z = parseFloat(dampSlider.value);
      parkAnim.setDamping(z);
      if (dampValue) dampValue.textContent = z.toFixed(2);
    });
  }

  /* ── Block Diagram ── */
  if (window.PLLDiagram) {
    window.PLLDiagram.init();
  }

  /* ── Canvas Resize Handler ── */
  let resizeTimer;
  window.addEventListener('resize', () => {
    clearTimeout(resizeTimer);
    resizeTimer = setTimeout(() => {
      gridAnim.resize(); gridAnim.draw();
      spwmAnim.resize(); spwmAnim.draw();
      clarkeAnim.resize(); clarkeAnim.draw();
      parkAnim.resize(); parkAnim.draw();
    }, 200);
  });

  /* ── Theme Change Observer ── */
  const observer = new MutationObserver(() => {
    gridAnim.draw();
    spwmAnim.draw();
    clarkeAnim.draw();
    parkAnim.draw();
    if (window.PLLDiagram) window.PLLDiagram.init();
  });

  observer.observe(document.documentElement, {
    attributes: true,
    attributeFilter: ['data-theme'],
  });

  /* ── Reduced Motion ── */
  if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
    gridAnim.stop(); spwmAnim.stop(); clarkeAnim.stop(); parkAnim.stop();
    gridAnim.draw(); spwmAnim.draw(); clarkeAnim.draw(); parkAnim.draw();
  }
});
