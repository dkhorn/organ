// play_page.h — Channel Player UI
// One button per configured output; hold to turn on, release to turn off.
// Buttons are color-coded by MIDI mapping count: green=1, yellow=multi, red=none.
#pragma once

static const char PLAY_PAGE_HTML[] = R"rawhtml(<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Channel Player</title>
<style>
  * { box-sizing: border-box; }
  body { font-family: Arial, sans-serif; margin: 0; padding: 12px; background: #1a1a2e; color: #eee; }
  h1 { margin: 0 0 4px; font-size: 1.3em; color: #e0e0ff; }
  .nav { margin-bottom: 12px; font-size: 0.85em; }
  .nav a { color: #7eb8f7; text-decoration: none; margin-right: 12px; }
  .nav a:hover { text-decoration: underline; }
  #status { font-size: 0.8em; color: #aaa; min-height: 1.2em; margin-bottom: 8px; }
  #all-off {
    display: block; width: 100%; padding: 14px;
    background: #7b1a1a; color: #fcc; border: 2px solid #c44;
    border-radius: 8px; font-size: 1.1em; font-weight: bold;
    cursor: pointer; margin-bottom: 14px; touch-action: none;
  }
  #all-off:active { filter: brightness(1.6); }
  #grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(84px, 1fr));
    gap: 8px;
  }
  .ch-btn {
    padding: 8px 4px;
    min-height: 62px;
    border: 2px solid;
    border-radius: 8px;
    font-size: 0.9em;
    text-align: center;
    cursor: pointer;
    user-select: none;
    -webkit-user-select: none;
    touch-action: none;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: flex-start;
    transition: filter 0.05s;
  }
  /* Mapping count colour states */
  .ch-btn.none  { background: #4a1010; border-color: #933; color: #fbb; }
  .ch-btn.one   { background: #0f3320; border-color: #3a7; color: #bfb; }
  .ch-btn.multi { background: #3a2e00; border-color: #996; color: #ffd; }
  /* Pressed / active */
  .ch-btn.active { filter: brightness(2.0); }
  .ch-num { font-size: 1.15em; font-weight: bold; line-height: 1.4; }
  .ch-midi {
    font-size: 0.65em;
    line-height: 1.35;
    margin-top: 3px;
    white-space: pre;   /* keep one mapping per line */
    opacity: 0.92;
  }
</style>
</head>
<body>
<h1>Channel Player</h1>
<div class="nav">
  <a href="/">Home</a>
  <a href="/config">Config</a>
  <a href="/logs">Logs</a>
</div>
<div id="status">Loading...</div>
<button id="all-off" onpointerdown="allOff()">&#9632; All Off</button>
<div id="grid"></div>
<script>
const NOTE_NAMES = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
function noteName(n) { return NOTE_NAMES[n % 12] + (Math.floor(n / 12) - 1); }

// Returns array of length num_outputs; each element is an array of label strings
// like ["Ch0:C4", "Ch2:D#5"] for every (enabled channel, note) that maps to that output.
function buildMappings(cfg) {
  const maps = Array.from({length: cfg.num_outputs}, () => []);
  for (let ch = 0; ch < 16; ch++) {
    if (!cfg.channels[ch].enabled) continue;
    const map = cfg.channels[ch].map;
    for (let note = 0; note < 128; note++) {
      const out = map[note];
      if (out >= 0 && out < cfg.num_outputs) {
        maps[out].push('Ch' + ch + ':' + noteName(note));
      }
    }
  }
  return maps;
}

async function load() {
  try {
    const r = await fetch('/config/get');
    const cfg = await r.json();
    buildGrid(cfg);
    document.getElementById('status').textContent =
      cfg.num_outputs + ' outputs. \u25a0=none \u25a0=one \u25a0=multi MIDI input';
    // colour the legend squares in the status line
    const st = document.getElementById('status');
    st.innerHTML = cfg.num_outputs + ' outputs. ' +
      '<span style="color:#f88">&#9632;</span>=none &nbsp;' +
      '<span style="color:#6d6">&#9632;</span>=1 MIDI &nbsp;' +
      '<span style="color:#fd6">&#9632;</span>=multi MIDI' +
      ' &mdash; hold to activate';
  } catch(e) {
    document.getElementById('status').textContent = 'Error loading config: ' + e;
  }
}

function buildGrid(cfg) {
  const mappings = buildMappings(cfg);
  const grid = document.getElementById('grid');
  grid.innerHTML = '';

  for (let i = 0; i < cfg.num_outputs; i++) {
    const maps = mappings[i];
    const colorCls = maps.length === 0 ? 'none' : maps.length === 1 ? 'one' : 'multi';

    const btn = document.createElement('div');
    btn.className = 'ch-btn ' + colorCls;
    btn.dataset.ch = i;

    const numEl = document.createElement('div');
    numEl.className = 'ch-num';
    numEl.textContent = i;
    btn.appendChild(numEl);

    if (maps.length > 0) {
      const mEl = document.createElement('div');
      mEl.className = 'ch-midi';
      mEl.textContent = maps.join('\n');
      btn.appendChild(mEl);
    }

    btn.addEventListener('pointerdown', e => {
      e.preventDefault();
      btn.setPointerCapture(e.pointerId);
      noteOn(i, btn);
    });
    btn.addEventListener('pointerup',     () => noteOff(i, btn));
    btn.addEventListener('pointercancel', () => noteOff(i, btn));
    btn.addEventListener('pointerleave',  () => { if (btn.classList.contains('active')) noteOff(i, btn); });

    grid.appendChild(btn);
  }
}

function noteOn(ch, btn) {
  if (btn.classList.contains('active')) return;
  btn.classList.add('active');
  fetch('/note_on_by_index?note=' + ch).catch(() => {});
}

function noteOff(ch, btn) {
  if (!btn.classList.contains('active')) return;
  btn.classList.remove('active');
  fetch('/note_off_by_index?note=' + ch).catch(() => {});
}

function allOff() {
  document.querySelectorAll('.ch-btn.active').forEach(b => b.classList.remove('active'));
  fetch('/all_off').catch(() => {});
}

load();
</script>
</body>
</html>
)rawhtml";
