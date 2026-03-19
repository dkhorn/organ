#ifndef CONFIG_PAGE_H
#define CONFIG_PAGE_H

const char CONFIG_PAGE_HTML[] = R"CFGPAGE(
<!DOCTYPE html>
<html>
<head>
<title>Windchest Config</title>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
* { box-sizing: border-box; }
body { font-family: monospace; background:#1e1e1e; color:#d4d4d4; margin:0; padding:16px; }
h1 { color:#4ec9b0; margin:0 0 16px; }
h2 { color:#9cdcfe; margin:14px 0 8px; font-size:15px; }
.section { background:#252526; border-radius:6px; padding:14px; margin-bottom:14px; }
.row { display:flex; align-items:center; gap:10px; margin:6px 0; flex-wrap:wrap; }
label { color:#ce9178; }
input[type=number] { background:#3c3c3c; color:#d4d4d4; border:1px solid #555;
  padding:3px 6px; border-radius:3px; }
input[type=checkbox] { width:15px; height:15px; cursor:pointer; }
button { background:#0e639c; color:#fff; border:none; padding:5px 13px;
  border-radius:3px; cursor:pointer; font-family:monospace; }
button:hover { background:#1177bb; }
button.warn { background:#7a3030; }
button.warn:hover { background:#a03333; }
.tabs { display:flex; flex-wrap:wrap; gap:4px; margin:8px 0; }
.tabs button { min-width:34px; padding:3px 7px; background:#3c3c3c;
  border:2px solid transparent; font-size:12px; }
.tabs button.active { background:#0e639c; }
.tabs button.on { border-color:#4ec9b0; }
.status { color:#4ec9b0; font-size:12px; margin-left:6px; }
table { border-collapse:collapse; width:100%; margin-top:8px; font-size:12px; }
th { background:#1a1a1a; padding:4px 6px; text-align:left; position:sticky; top:0; }
td { padding:2px 4px; border-bottom:1px solid #2a2a2a; }
tr:hover td { background:#2d2d2d; }
.nn { color:#4ec9b0; display:inline-block; width:38px; }
.nn.sharp { color:#888; }
input.out { width:46px; background:#3c3c3c; color:#d4d4d4; border:1px solid #555;
  padding:2px 4px; border-radius:2px; text-align:center; }
input.out.mapped { border-color:#4ec9b0; }
input.out.unmapped { color:#555; }
</style>
</head>
<body>
<h1>Windchest Config</h1>

<div class="section">
  <h2>Hardware</h2>
  <div class="row">
    <label>Number of outputs:</label>
    <input type="number" id="num_outputs" min="1" max="128" style="width:60px">
    <button onclick="saveNumOutputs()">Save</button>
    <span class="status" id="hw_st"></span>
  </div>
</div>

<div class="section">
  <h2>MIDI Channel Mapping</h2>
  <div style="color:#888;font-size:12px;margin-bottom:8px">
    Select a MIDI channel tab, enable it, and map each MIDI note to a hardware output index (0-based).
    Use -1 to ignore a note. Teal border = enabled channel.
  </div>
  <div class="tabs" id="ch_tabs"></div>
  <div id="ch_panel">
    <div class="row">
      <label><input type="checkbox" id="ch_en"> Enable this MIDI channel</label>
      <div style="flex:1"></div>
      <label>Auto-map from MIDI note:</label>
      <input type="number" id="auto_base" value="34" min="0" max="127" style="width:52px">
      <button onclick="autoMap()">Apply</button>
      <button onclick="clearMap()" class="warn">Clear all</button>
      <button onclick="saveChannel()">Save channel</button>
      <span class="status" id="ch_st"></span>
    </div>
    <table>
      <thead>
        <tr>
          <th>#</th><th>Note</th><th>Output</th>
        </tr>
      </thead>
      <tbody id="note_body"></tbody>
    </table>
  </div>
</div>

<script>
let cfg = null;
let curCh = 0;
const NAMES = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];

function noteName(n) {
  return NAMES[n % 12] + (Math.floor(n / 12) - 1);
}
function isSharp(n) { return NAMES[n % 12].includes('#'); }

async function load() {
  try {
    const r = await fetch('/config/get');
    cfg = await r.json();
    document.getElementById('num_outputs').value = cfg.num_outputs;
    renderTabs();
    renderChannel(curCh);
  } catch(e) {
    document.body.insertAdjacentHTML('afterbegin',
      '<div style="color:red;padding:8px">Error loading config: ' + e + '</div>');
  }
}

function renderTabs() {
  const el = document.getElementById('ch_tabs');
  el.innerHTML = '';
  for (let i = 0; i < 16; i++) {
    const b = document.createElement('button');
    b.textContent = i;
    if (i === curCh) b.classList.add('active');
    if (cfg.channels[i].enabled) b.classList.add('on');
    b.onclick = (function(ch){ return function(){ curCh=ch; renderTabs(); renderChannel(ch); }; })(i);
    el.appendChild(b);
  }
}

function makeRow(ch, n) {
  const nn = noteName(n);
  const sharp = isSharp(n);
  const val = cfg.channels[ch].map[n];
  const cls = 'out' + (val >= 0 ? ' mapped' : ' unmapped');
  return `<tr><td>${n}</td>` +
    `<td><span class="nn${sharp?' sharp':''}">${nn}</span></td>` +
    `<td><input class="${cls}" type="number" min="-1" max="127" value="${val}" ` +
    `onchange="setNote(${ch},${n},this)"></td></tr>`;
}

function renderChannel(ch) {
  document.getElementById('ch_en').checked = cfg.channels[ch].enabled;
  const tbody = document.getElementById('note_body');
  let html = '';
  for (let n = 0; n < 128; n++) html += makeRow(ch, n);
  tbody.innerHTML = html;
}

function setNote(ch, n, input) {
  const v = parseInt(input.value);
  cfg.channels[ch].map[n] = isNaN(v) ? -1 : v;
  input.className = 'out' + (v >= 0 ? ' mapped' : ' unmapped');
}

async function saveNumOutputs() {
  const val = document.getElementById('num_outputs').value;
  cfg.num_outputs = parseInt(val);
  const r = await fetch('/config/num_outputs', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'value=' + val
  });
  flash('hw_st', await r.text());
}

async function saveChannel() {
  const ch = curCh;
  cfg.channels[ch].enabled = document.getElementById('ch_en').checked;
  const body = 'ch=' + ch +
    '&enabled=' + (cfg.channels[ch].enabled ? 1 : 0) +
    '&map=' + cfg.channels[ch].map.join(',');
  const r = await fetch('/config/channel', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: body
  });
  flash('ch_st', await r.text());
  renderTabs();
}

function autoMap() {
  const base = parseInt(document.getElementById('auto_base').value);
  const num = cfg.num_outputs;
  const ch = curCh;
  for (let n = 0; n < 128; n++) {
    const out = n - base;
    cfg.channels[ch].map[n] = (out >= 0 && out < num) ? out : -1;
  }
  renderChannel(ch);
}

function clearMap() {
  cfg.channels[curCh].map.fill(-1);
  renderChannel(curCh);
}

function flash(id, msg) {
  const el = document.getElementById(id);
  el.textContent = msg;
  setTimeout(() => el.textContent = '', 3000);
}

load();
</script>
</body>
</html>
)CFGPAGE";

#endif // CONFIG_PAGE_H
