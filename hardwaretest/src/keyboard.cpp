// keyboard.cpp
#include "keyboard.h"

// HTML for a 20-key keyboard starting at A (MIDI note 69 = A440)
static const char keyboard_html[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background: #2c2c2c;
      color: white;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
    }
    h1 { color: #fff; margin-bottom: 30px; }
    .keyboard {
      display: flex;
      gap: 4px;
      background: #1a1a1a;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 5px 15px rgba(0,0,0,0.5);
    }
    .key {
      width: 50px;
      height: 200px;
      background: white;
      border: 2px solid #333;
      border-radius: 0 0 5px 5px;
      cursor: pointer;
      transition: all 0.1s;
      display: flex;
      align-items: flex-end;
      justify-content: center;
      padding-bottom: 10px;
      font-size: 12px;
      color: #333;
      user-select: none;
      -webkit-user-select: none;
      touch-action: none;
    }
    .key:hover {
      background: #f0f0f0;
    }
    .key:active, .key.pressed {
      background: #4CAF50;
      color: white;
      transform: translateY(2px);
    }
    .black {
      background: #222;
      color: #888;
      height: 130px;
      width: 35px;
      margin: 0 -19px;
      z-index: 1;
      border: 2px solid #000;
    }
    .black:hover {
      background: #333;
    }
    .black:active, .black.pressed {
      background: #4CAF50;
      color: white;
    }
    .info {
      margin-top: 20px;
      text-align: center;
      color: #888;
    }
    .nav {
      margin-top: 30px;
      padding: 15px;
      background: #1a1a1a;
      border-radius: 8px;
      text-align: center;
    }
    .nav a {
      color: #4CAF50;
      text-decoration: none;
      margin: 0 15px;
      font-size: 14px;
      padding: 8px 16px;
      border: 1px solid #4CAF50;
      border-radius: 4px;
      transition: all 0.2s;
      display: inline-block;
    }
    .nav a:hover {
      background: #4CAF50;
      color: white;
    }
    .song-controls {
      margin-top: 20px;
      display: flex;
      gap: 10px;
      justify-content: center;
      flex-wrap: wrap;
    }
    .song-btn {
      padding: 12px 24px;
      font-size: 16px;
      background-color: #2196F3;
      color: white;
      border: none;
      border-radius: 6px;
      cursor: pointer;
      font-weight: bold;
      box-shadow: 0 2px 8px rgba(0,0,0,0.2);
      transition: all 0.2s;
    }
    .song-btn:hover {
      background-color: #0b7dda;
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(0,0,0,0.3);
    }
    .song-btn:active {
      transform: translateY(0);
    }
    .params-controls {
      margin-top: 15px;
      display: flex;
      gap: 20px;
      justify-content: center;
      align-items: center;
      flex-wrap: wrap;
    }
    .param-group {
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .param-group label {
      font-weight: bold;
      color: #333;
    }
    .param-group input {
      width: 80px;
      padding: 6px;
      font-size: 14px;
      border: 2px solid #ccc;
      border-radius: 4px;
    }
  </style>
</head>
<body>
  <h1>MIDI Keyboard</h1>
  <div class='keyboard' id='keyboard'></div>
  
  <div class='song-controls'>
    <button class='song-btn' onclick='playSong(0)'>Velocity Test</button>
    <button class='song-btn' onclick='playSong(1)'>C Major Scale</button>
    <button class='song-btn' onclick='playSong(2)'>Chromatic</button>
    <button class='song-btn' onclick='playSong(3)'>Imported Melody</button>
    <button class='song-btn' onclick='playSong(4)'>Ode to Joy</button>
    <button class='song-btn' onclick='playSong(5)'>Canon in D</button>
    <button class='song-btn' onclick='playSong(6)'>Greensleeves</button>
    <button class='song-btn' onclick='playSong(7)'>Wind Chimes</button>
  </div>
  
  <div class='params-controls'>
    <div class='param-group'>
      <label for='transpose'>Transpose:</label>
      <input type='number' id='transpose' value='0' min='-24' max='24' />
      <span>semitones</span>
    </div>
    <div class='param-group'>
      <label for='tempo'>Tempo:</label>
      <input type='number' id='tempo' value='120' min='20' max='300' />
      <span>BPM</span>
    </div>
    <div class='param-group'>
      <label for='velocity'>Velocity:</label>
      <input type='number' id='velocity' value='127' min='1' max='127' />
      <span>(1-127)</span>
    </div>
    <div class='param-group'>
      <label for='repeatMode'>Repeat Mode:</label>
      <input type='checkbox' id='repeatMode' style='width: auto; transform: scale(1.5); margin-left: 10px;' />
      <span style='margin-left: 10px;'>OFF</span>
    </div>
  </div>
  
  <button id='panicBtn' style='margin-top: 30px; padding: 15px 40px; font-size: 18px; background-color: #dc3545; color: white; border: none; border-radius: 8px; cursor: pointer; font-weight: bold; box-shadow: 0 3px 10px rgba(0,0,0,0.3);'>
    PANIC - All Stop
  </button>
  <div class='info'>
    <p>20 keys starting at A440 (MIDI note 69)</p>
    <p>Click and hold keys to play • Repeat Mode: Click=strike, Release=repeat at that tempo</p>
  </div>

  <script>
    // MIDI note 69 = A440, we have 20 keys
    const START_NOTE = 69;
    const NUM_KEYS = 20;
    
    // Key pattern: W=white, B=black
    // Starting from A: A, A#, B, C, C#, D, D#, E, F, F#, G, G#, (next A)
    const pattern = ['W','B','W','W','B','W','B','W','W','B','W','B'];
    const noteNames = ['A','A#','B','C','C#','D','D#','E','F','F#','G','G#'];
    
    const keyboard = document.getElementById('keyboard');
    const activeNotes = new Set();
    const repeatingNotes = new Map(); // Maps note -> {velocity, downTime, ignoreUp}
    
    // Update repeat mode label
    const repeatModeCheckbox = document.getElementById('repeatMode');
    repeatModeCheckbox.addEventListener('change', (e) => {
      const label = e.target.parentElement.querySelector('span:last-child');
      label.textContent = e.target.checked ? 'ON' : 'OFF';
      label.style.color = e.target.checked ? '#4CAF50' : '';
      label.style.fontWeight = e.target.checked ? 'bold' : '';
    });
    
    // Create keys
    for (let i = 0; i < NUM_KEYS; i++) {
      const midiNote = START_NOTE + i;
      const patternIdx = i % 12;
      const isBlack = pattern[patternIdx] === 'B';
      const noteName = noteNames[patternIdx];
      const octave = Math.floor((START_NOTE + i) / 12) - 1;
      
      const key = document.createElement('div');
      key.className = 'key' + (isBlack ? ' black' : '');
      key.textContent = noteName;
      key.dataset.note = midiNote;
      
      // Mouse events
      key.addEventListener('mousedown', (e) => {
        e.preventDefault();
        const velocity = calculateVelocity(e, key);
        const isRepeatMode = document.getElementById('repeatMode').checked;
        
        if (isRepeatMode) {
          // Check if note is already repeating
          if (repeatingNotes.has(midiNote)) {
            // Stop repeating this note
            fetch('/repeat/stop?note=' + midiNote)
              .catch(err => console.error('Error:', err));
            repeatingNotes.delete(midiNote);
            key.classList.remove('pressed');
            return; // Ignore mouse-up for this click
          } else {
            // Ring once and note the time
            noteOn(midiNote, key, velocity);
            noteOff(midiNote, key);
            repeatingNotes.set(midiNote, {
              velocity: velocity,
              downTime: Date.now(),
              ignoreUp: false
            });
          }
        } else {
          // Normal mode
          noteOn(midiNote, key, velocity);
        }
      });
      
      key.addEventListener('mouseup', (e) => {
        e.preventDefault();
        const isRepeatMode = document.getElementById('repeatMode').checked;
        
        if (isRepeatMode) {
          // Check if we should start repeating
          if (repeatingNotes.has(midiNote)) {
            const info = repeatingNotes.get(midiNote);
            if (info.ignoreUp) {
              // This was a stop-repeat click, clear the flag
              info.ignoreUp = false;
              return;
            }
            
            // Calculate period from time held
            const period = Date.now() - info.downTime;
            if (period >= 100) { // Minimum 100ms period
              // Start repeating
              fetch('/repeat/start?note=' + midiNote + '&velocity=' + info.velocity + '&period=' + period + '&count=0')
                .catch(err => console.error('Error:', err));
              key.classList.add('pressed'); // Keep it green
            } else {
              // Too short, just clear state
              repeatingNotes.delete(midiNote);
            }
          }
        } else {
          // Normal mode
          noteOff(midiNote, key);
        }
      });
      
      key.addEventListener('mouseleave', (e) => {
        const isRepeatMode = document.getElementById('repeatMode').checked;
        if (!isRepeatMode && activeNotes.has(midiNote)) {
          noteOff(midiNote, key);
        }
      });
      
      // Touch events
      key.addEventListener('touchstart', (e) => {
        e.preventDefault();
        const touch = e.touches[0];
        const velocity = calculateVelocityTouch(touch, key);
        const isRepeatMode = document.getElementById('repeatMode').checked;
        
        if (isRepeatMode) {
          // Check if note is already repeating
          if (repeatingNotes.has(midiNote)) {
            // Stop repeating this note
            fetch('/repeat/stop?note=' + midiNote)
              .catch(err => console.error('Error:', err));
            repeatingNotes.delete(midiNote);
            key.classList.remove('pressed');
            return; // Ignore touch-end for this touch
          } else {
            // Ring once and note the time
            noteOn(midiNote, key, velocity);
            noteOff(midiNote, key);
            repeatingNotes.set(midiNote, {
              velocity: velocity,
              downTime: Date.now(),
              ignoreUp: false
            });
          }
        } else {
          // Normal mode
          noteOn(midiNote, key, velocity);
        }
      });
      
      key.addEventListener('touchend', (e) => {
        e.preventDefault();
        const isRepeatMode = document.getElementById('repeatMode').checked;
        
        if (isRepeatMode) {
          // Check if we should start repeating
          if (repeatingNotes.has(midiNote)) {
            const info = repeatingNotes.get(midiNote);
            if (info.ignoreUp) {
              // This was a stop-repeat touch, clear the flag
              info.ignoreUp = false;
              return;
            }
            
            // Calculate period from time held
            const period = Date.now() - info.downTime;
            if (period >= 100) { // Minimum 100ms period
              // Start repeating
              fetch('/repeat/start?note=' + midiNote + '&velocity=' + info.velocity + '&period=' + period + '&count=0')
                .catch(err => console.error('Error:', err));
              key.classList.add('pressed'); // Keep it green
            } else {
              // Too short, just clear state
              repeatingNotes.delete(midiNote);
            }
          }
        } else {
          // Normal mode
          noteOff(midiNote, key);
        }
      });
      
      keyboard.appendChild(key);
    }
    
    // Calculate velocity based on mouse click position
    // Bottom of key = 127, top of key = 10
    function calculateVelocity(e, element) {
      const rect = element.getBoundingClientRect();
      const y = e.clientY - rect.top; // Click position relative to top of key
      const height = rect.height;
      
      // Map position to velocity: top (y=0) = 10, bottom (y=height) = 127
      const velocity = Math.round(10 + (y / height) * (127 - 10));
      return Math.max(10, Math.min(127, velocity));
    }
    
    // Calculate velocity based on touch position
    function calculateVelocityTouch(touch, element) {
      const rect = element.getBoundingClientRect();
      const y = touch.clientY - rect.top;
      const height = rect.height;
      
      const velocity = Math.round(10 + (y / height) * (127 - 10));
      return Math.max(10, Math.min(127, velocity));
    }
    
    function noteOn(note, element, velocity) {
      if (activeNotes.has(note)) return;
      activeNotes.add(note);
      element.classList.add('pressed');
      
      // Use calculated velocity from click position
      fetch('/note_on?note=' + note + '&velocity=' + velocity)
        .catch(e => console.error('Error:', e));
    }
    
    function noteOff(note, element) {
      if (!activeNotes.has(note)) return;
      activeNotes.delete(note);
      element.classList.remove('pressed');
      
      fetch('/note_off?note=' + note + '&velocity=64')
        .catch(e => console.error('Error:', e));
    }
    
    // Global mouse up to catch releases outside keys
    document.addEventListener('mouseup', () => {
      activeNotes.forEach(note => {
        const key = keyboard.querySelector(`[data-note="${note}"]`);
        if (key) noteOff(note, key);
      });
    });
    
    // Song playback function
    function playSong(songId) {
      const transpose = document.getElementById('transpose').value;
      const tempo = document.getElementById('tempo').value;
      let url = '/play/' + songId + '?transpose=' + transpose + '&tempo=' + tempo;
      
      fetch(url)
        .then(r => r.text())
        .then(data => console.log('Playing song ' + songId + ':', data))
        .catch(e => console.error('Song error:', e));
    }
    
    // Panic button handler - stops sequencer AND all notes
    document.getElementById('panicBtn').addEventListener('click', () => {
      // Stop sequencer first
      fetch('/seq_stop')
        .then(() => fetch('/all_off'))
        .then(r => r.text())
        .then(data => console.log('Panic:', data))
        .catch(e => console.error('Panic error:', e));
      
      // Clear all visual feedback
      activeNotes.forEach(note => {
        const key = keyboard.querySelector(`[data-note="${note}"]`);
        if (key) key.classList.remove('pressed');
      });
      activeNotes.clear();
      
      // Clear repeating notes state
      repeatingNotes.forEach((info, note) => {
        const key = keyboard.querySelector(`[data-note="${note}"]`);
        if (key) key.classList.remove('pressed');
      });
      repeatingNotes.clear();
    });
  </script>
  
  <div class="nav">
    <a href="/channels">Channels</a>
    <a href="/settings">Settings</a>
    <a href="/api">API Docs</a>
  </div>
</body>
</html>
)rawliteral";

extern "C" {

const char* get_keyboard_html() {
  return keyboard_html;
}

} // extern "C"
