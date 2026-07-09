/**
 * PLL Blog — SVG Block Diagram
 * SRF-PLL block diagram with animated signal flow dots and hover tooltips.
 */

function initDiagram() {
  const container = document.getElementById('pllDiagram');
  if (!container) return;

  const c = getComputedStyle(document.documentElement);
  const accent = c.getPropertyValue('--accent-green').trim() || '#3ddc84';
  const accentBlue = c.getPropertyValue('--accent-blue').trim() || '#4da6ff';
  const accentOrange = c.getPropertyValue('--accent-orange').trim() || '#ff8c42';
  const textColor = c.getPropertyValue('--text-primary').trim() || '#e2f0e8';
  const textMuted = c.getPropertyValue('--text-muted').trim() || '#3f5e4a';
  const borderColor = c.getPropertyValue('--border').trim() || 'rgba(61,220,132,0.1)';

  // SVG dimensions
  const svgW = 780;
  const svgH = 280;

  // Block definitions
  const blocks = [
    {
      id: 'abc_ab', x: 30, y: 100, w: 100, h: 55,
      label: 'abc → αβ', sublabel: 'Clarke',
      color: accentBlue,
      tooltip: 'Clarke Transform: Converts 3-phase (abc) to stationary 2-phase (αβ) frame.\nvα = va − ½vb − ½vc\nvβ = (√3/2)(vb − vc)',
    },
    {
      id: 'ab_dq', x: 180, y: 100, w: 100, h: 55,
      label: 'αβ → dq', sublabel: 'Park',
      color: accent,
      tooltip: 'Park Transform: Rotates αβ frame by estimated angle θ̂ to get dq components.\nvd = vα cos(θ̂) + vβ sin(θ̂)\nvq = −vα sin(θ̂) + vβ cos(θ̂)',
    },
    {
      id: 'pi', x: 360, y: 100, w: 100, h: 55,
      label: 'PI', sublabel: 'Controller',
      color: accentOrange,
      tooltip: 'PI Controller: Drives vq → 0 by adjusting frequency estimate.\nOutput = Kp·vq + Ki·∫vq dt\nKp = 2ζωₙ,  Ki = ωₙ²',
    },
    {
      id: 'sum', x: 530, y: 100, w: 50, h: 55,
      label: 'Σ', sublabel: '',
      color: textMuted,
      tooltip: 'Summation: Adds feedforward frequency (ω₀ = 2π·50) to PI output.\nω̂ = ω₀ + Δω',
    },
    {
      id: 'vco', x: 640, y: 100, w: 100, h: 55,
      label: '1/s', sublabel: 'VCO',
      color: accentBlue,
      tooltip: 'Voltage Controlled Oscillator: Integrates frequency to get angle.\nθ̂ = ∫ω̂ dt\nThis estimated angle feeds back to the Park transform.',
    },
  ];

  // Connection paths (from → to), with intermediate points for routing
  const connections = [
    { from: 'abc_ab', to: 'ab_dq', label: 'vαβ' },
    { from: 'ab_dq', to: 'pi', label: 'vq' },
    { from: 'pi', to: 'sum', label: 'Δω' },
    { from: 'sum', to: 'vco', label: 'ω̂' },
    // Feedback: VCO output → Park transform (bottom route)
    { from: 'vco', to: 'ab_dq', label: 'θ̂', feedback: true },
  ];

  // Feedforward label
  const ffLabel = { x: 555, y: 80, text: 'ω₀' };

  // Build SVG
  let svg = `<svg viewBox="0 0 ${svgW} ${svgH}" xmlns="http://www.w3.org/2000/svg" style="font-family: JetBrains Mono, monospace;">`;

  // Defs for glow filter and arrow marker
  svg += `
    <defs>
      <filter id="blockGlow" x="-20%" y="-20%" width="140%" height="140%">
        <feGaussianBlur stdDeviation="3" result="blur"/>
        <feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge>
      </filter>
      <marker id="arrowHead" markerWidth="8" markerHeight="6" refX="8" refY="3" orient="auto">
        <path d="M0,0 L8,3 L0,6 Z" fill="${textMuted}"/>
      </marker>
      <marker id="arrowHeadFeedback" markerWidth="8" markerHeight="6" refX="8" refY="3" orient="auto">
        <path d="M0,0 L8,3 L0,6 Z" fill="${accent}"/>
      </marker>
    </defs>
  `;

  // Input arrow (grid voltages entering first block)
  svg += `
    <line x1="0" y1="127" x2="30" y2="127" stroke="${textMuted}" stroke-width="1.5" marker-end="url(#arrowHead)"/>
    <text x="2" y="118" fill="${textMuted}" font-size="10">v_abc</text>
  `;

  // Draw connections first (behind blocks)
  for (const conn of connections) {
    const fromBlock = blocks.find(b => b.id === conn.from);
    const toBlock = blocks.find(b => b.id === conn.to);
    if (!fromBlock || !toBlock) continue;

    if (conn.feedback) {
      // Feedback path: goes down from VCO, runs along bottom, up into Park
      const startX = fromBlock.x + fromBlock.w / 2;
      const startY = fromBlock.y + fromBlock.h;
      const endX = toBlock.x + toBlock.w / 2;
      const endY = toBlock.y + toBlock.h;
      const bottomY = svgH - 30;

      const pathId = `path-${conn.from}-${conn.to}`;
      svg += `
        <path id="${pathId}"
          d="M${startX},${startY} L${startX},${bottomY} L${endX},${bottomY} L${endX},${endY}"
          fill="none" stroke="${accent}" stroke-width="1.5" stroke-dasharray="6,3"
          marker-end="url(#arrowHeadFeedback)"/>
        <text x="${(startX + endX) / 2}" y="${bottomY - 6}" fill="${accent}" font-size="9.5" text-anchor="middle">${conn.label}</text>
      `;

      // Signal dot on feedback path
      svg += `
        <circle r="3.5" fill="${accent}" opacity="0.9">
          <animateMotion dur="3s" repeatCount="indefinite">
            <mpath href="#${pathId}"/>
          </animateMotion>
        </circle>
      `;
    } else {
      // Forward path: straight horizontal
      const startX = fromBlock.x + fromBlock.w;
      const startY = fromBlock.y + fromBlock.h / 2;
      const endX = toBlock.x;
      const endY = toBlock.y + toBlock.h / 2;

      const pathId = `path-${conn.from}-${conn.to}`;
      svg += `
        <line id="${pathId}" x1="${startX}" y1="${startY}" x2="${endX}" y2="${endY}"
          stroke="${textMuted}" stroke-width="1.5" marker-end="url(#arrowHead)"/>
        <text x="${(startX + endX) / 2}" y="${startY - 8}" fill="${textMuted}" font-size="9.5" text-anchor="middle">${conn.label}</text>
      `;

      // Signal dot
      svg += `
        <circle r="3" fill="${accentBlue}" opacity="0.8">
          <animate attributeName="cx" from="${startX}" to="${endX}" dur="1.5s" repeatCount="indefinite"/>
          <animate attributeName="cy" from="${startY}" to="${endY}" dur="1.5s" repeatCount="indefinite"/>
        </circle>
      `;
    }
  }

  // Feedforward arrow into sum
  const sumBlock = blocks.find(b => b.id === 'sum');
  svg += `
    <line x1="${sumBlock.x + sumBlock.w / 2}" y1="70" x2="${sumBlock.x + sumBlock.w / 2}" y2="${sumBlock.y}"
      stroke="${textMuted}" stroke-width="1.5" marker-end="url(#arrowHead)"/>
    <text x="${ffLabel.x}" y="${ffLabel.y - 10}" fill="${textMuted}" font-size="9.5" text-anchor="middle">${ffLabel.text}</text>
  `;

  // Draw blocks
  for (const b of blocks) {
    svg += `
      <g class="diagram-block" data-id="${b.id}" data-tooltip="${b.tooltip.replace(/"/g, '&quot;')}" style="cursor:pointer">
        <rect x="${b.x}" y="${b.y}" width="${b.w}" height="${b.h}"
          rx="8" ry="8"
          fill="rgba(0,0,0,0.3)" stroke="${b.color}" stroke-width="1.5"
          style="transition: stroke-width 0.2s, filter 0.2s"/>
        <text x="${b.x + b.w / 2}" y="${b.y + b.h / 2 - (b.sublabel ? 4 : 0)}"
          fill="${textColor}" font-size="12" font-weight="600" text-anchor="middle" dominant-baseline="middle">
          ${b.label}
        </text>
    `;
    if (b.sublabel) {
      svg += `
        <text x="${b.x + b.w / 2}" y="${b.y + b.h / 2 + 14}"
          fill="${textMuted}" font-size="9" text-anchor="middle" dominant-baseline="middle">
          ${b.sublabel}
        </text>
      `;
    }
    svg += `</g>`;
  }

  // Output arrow
  const vcoBlock = blocks.find(b => b.id === 'vco');
  svg += `
    <line x1="${vcoBlock.x + vcoBlock.w}" y1="${vcoBlock.y + vcoBlock.h / 2}"
      x2="${svgW}" y2="${vcoBlock.y + vcoBlock.h / 2}"
      stroke="${textMuted}" stroke-width="1.5" marker-end="url(#arrowHead)"/>
    <text x="${svgW - 10}" y="${vcoBlock.y + vcoBlock.h / 2 - 8}" fill="${textMuted}" font-size="10" text-anchor="end">θ̂</text>
  `;

  svg += '</svg>';

  container.innerHTML = svg;

  // ─── Tooltip handling ───
  const tooltip = document.createElement('div');
  tooltip.className = 'diagram-tooltip';
  container.style.position = 'relative';
  container.appendChild(tooltip);

  container.querySelectorAll('.diagram-block').forEach(g => {
    g.addEventListener('mouseenter', (e) => {
      const text = g.dataset.tooltip;
      tooltip.textContent = '';
      // Split by newline for multi-line
      text.split('\n').forEach((line, i) => {
        if (i > 0) tooltip.appendChild(document.createElement('br'));
        tooltip.appendChild(document.createTextNode(line));
      });
      tooltip.classList.add('visible');

      // Highlight block
      const rect = g.querySelector('rect');
      if (rect) {
        rect.style.strokeWidth = '2.5';
        rect.style.filter = 'url(#blockGlow)';
      }
    });

    g.addEventListener('mousemove', (e) => {
      const containerRect = container.getBoundingClientRect();
      tooltip.style.left = (e.clientX - containerRect.left + 12) + 'px';
      tooltip.style.top = (e.clientY - containerRect.top - 10) + 'px';
    });

    g.addEventListener('mouseleave', () => {
      tooltip.classList.remove('visible');
      const rect = g.querySelector('rect');
      if (rect) {
        rect.style.strokeWidth = '1.5';
        rect.style.filter = 'none';
      }
    });
  });
}

window.PLLDiagram = { init: initDiagram };
