#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

const char SETTINGS_PAGE_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Chimes Settings</title>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 900px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        h1 {
            color: #333;
            border-bottom: 3px solid #4CAF50;
            padding-bottom: 10px;
        }
        h2 {
            color: #4CAF50;
            margin-top: 30px;
            border-bottom: 2px solid #ddd;
            padding-bottom: 5px;
        }
        .section {
            background-color: white;
            padding: 20px;
            margin: 20px 0;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .setting-row {
            display: flex;
            align-items: center;
            margin: 15px 0;
            gap: 15px;
        }
        .setting-row label {
            flex: 0 0 200px;
            font-weight: bold;
            color: #555;
        }
        .setting-row input[type="number"],
        .setting-row input[type="text"],
        .setting-row select {
            flex: 1;
            padding: 8px;
            border: 2px solid #ddd;
            border-radius: 4px;
            font-size: 14px;
        }
        .setting-row input[type="checkbox"] {
            width: 20px;
            height: 20px;
        }
        .setting-row .unit {
            flex: 0 0 60px;
            color: #888;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            margin: 5px;
        }
        button:hover {
            background-color: #45a049;
        }
        button.secondary {
            background-color: #2196F3;
        }
        button.secondary:hover {
            background-color: #0b7dda;
        }
        button.danger {
            background-color: #f44336;
        }
        button.danger:hover {
            background-color: #da190b;
        }
        .button-group {
            margin-top: 20px;
            padding-top: 15px;
            border-top: 1px solid #ddd;
        }
        .status {
            padding: 10px;
            margin: 10px 0;
            border-radius: 4px;
            display: none;
        }
        .status.success {
            background-color: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .status.error {
            background-color: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        .nav {
            margin-top: 30px;
            padding: 15px;
            background: white;
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
        .info-text {
            color: #666;
            font-size: 12px;
            margin-top: 5px;
            font-style: italic;
        }
    </style>
</head>
<body>
    <h1>Chimes Settings</h1>
    
    <div id="status" class="status"></div>
    
    <div class="section">
        <h2>Clock Chimes</h2>
        <div class="setting-row">
            <label for="enabled">Master Enable</label>
            <input type="checkbox" id="enabled">
            <span class="info-text">Enable automatic clock chimes</span>
        </div>
        <div class="setting-row">
            <label for="tune">Chime Tune</label>
            <select id="tune">
                <option value="0">Disabled</option>
                <option value="1">Westminster</option>
                <option value="2">Whittington</option>
                <option value="3">St. Michael's</option>
                <option value="4">Winchester</option>
                <option value="5">Test Tune</option>
            </select>
            <span class="info-text">Select melody</span>
        </div>
        <div class="setting-row">
            <label for="tuneTempo">Tune Tempo</label>
            <input type="number" id="tuneTempo" min="60" max="240" value="120">
            <span class="unit">BPM</span>
        </div>
        <div class="setting-row">
            <label for="tuneVelocity">Tune Volume</label>
            <input type="number" id="tuneVelocity" min="1" max="127" value="127">
            <span class="unit">1-127</span>
        </div>
    </div>
    
    <div class="section">
        <h2>Hour Striking</h2>
        <div class="setting-row">
            <label for="hourStrike">Hour Strike Enable</label>
            <input type="checkbox" id="hourStrike">
            <span class="info-text">Strike bells to count the hour</span>
        </div>
        <div class="setting-row">
            <label for="hourNote1">Bell Note 1</label>
            <input type="number" id="hourNote1" min="0" max="127" value="70">
            <span class="unit">MIDI</span>
        </div>
        <div class="setting-row">
            <label for="hourNote2">Bell Note 2</label>
            <input type="number" id="hourNote2" min="0" max="127" value="77">
            <span class="unit">MIDI</span>
        </div>
        <div class="setting-row">
            <label for="hourNote3">Bell Note 3</label>
            <input type="number" id="hourNote3" min="0" max="127" value="0">
            <span class="unit">MIDI (0=off)</span>
        </div>
        <div class="setting-row">
            <label for="hourStrikeInterval">Strike Interval</label>
            <input type="number" id="hourStrikeInterval" min="500" max="5000" value="2000">
            <span class="unit">ms</span>
        </div>
        <div class="setting-row">
            <label for="hourVelocity">Hour Strike Volume</label>
            <input type="number" id="hourVelocity" min="1" max="127" value="127">
            <span class="unit">1-127</span>
        </div>
    </div>
    
    <div class="section">
        <h2>Quiet Mode</h2>
        <div class="setting-row">
            <label for="quietModeScale">Quiet Mode Scale</label>
            <input type="number" id="quietModeScale" min="0" max="127" value="64">
            <span class="unit">0-127</span>
        </div>
        <div class="info-text" style="margin-left: 215px;">Volume reduction during quiet hours (64 ≈ 50%)</div>
        <div class="setting-row">
            <label for="quietModeStartHour">Quiet Start Hour</label>
            <input type="number" id="quietModeStartHour" min="0" max="24" value="22">
            <span class="unit">0-24</span>
        </div>
        <div class="setting-row">
            <label for="quietModeEndHour">Quiet End Hour</label>
            <input type="number" id="quietModeEndHour" min="0" max="24" value="7">
            <span class="unit">0-24</span>
        </div>
        <div class="info-text" style="margin-left: 215px;">Supports wraparound (e.g., 22-7 = 10 PM to 7 AM)</div>
    </div>
    
    <div class="section">
        <h2>Silence Mode</h2>
        <div class="setting-row">
            <label for="silenceStartHour">Silence Start Hour</label>
            <input type="number" id="silenceStartHour" min="0" max="25" value="25">
            <span class="unit">0-25</span>
        </div>
        <div class="setting-row">
            <label for="silenceEndHour">Silence End Hour</label>
            <input type="number" id="silenceEndHour" min="0" max="25" value="25">
            <span class="unit">0-25</span>
        </div>
        <div class="info-text" style="margin-left: 215px;">No chimes at all during these hours (25 = disabled)</div>
    </div>
    
    <div class="section">
        <h2>Time Settings</h2>
        <div class="setting-row">
            <label for="ntpServer">NTP Server</label>
            <input type="text" id="ntpServer" value="pool.ntp.org">
            <span class="info-text"></span>
        </div>
        <div class="setting-row">
            <label for="timezoneOffset">Timezone Offset</label>
            <select id="timezoneOffset">
                <option value="-36000">UTC-10 (Hawaii)</option>
                <option value="-32400">UTC-9 (Alaska)</option>
                <option value="-28800">UTC-8 (Pacific)</option>
                <option value="-25200">UTC-7 (Mountain)</option>
                <option value="-21600">UTC-6 (Central)</option>
                <option value="-18000" selected>UTC-5 (Eastern)</option>
                <option value="-14400">UTC-4 (Atlantic)</option>
                <option value="0">UTC+0 (GMT)</option>
            </select>
            <span class="info-text">Seconds from UTC</span>
        </div>
        <div class="button-group">
            <button class="secondary" onclick="syncNTP()">Sync Time Now</button>
        </div>
    </div>
    
    <div class="button-group">
        <button onclick="saveAllSettings()">Save All Settings</button>
        <button class="secondary" onclick="loadSettings()">Refresh from Device</button>
        <button class="danger" onclick="testChime()">Test Chime (Hour)</button>
    </div>
    
    <div class="nav">
        <a href="/">Keyboard</a>
        <a href="/channels">Channels</a>
        <a href="/api">API Docs</a>
    </div>
    
    <script>
        function showStatus(message, isError = false) {
            const status = document.getElementById('status');
            status.textContent = message;
            status.className = 'status ' + (isError ? 'error' : 'success');
            status.style.display = 'block';
            setTimeout(() => status.style.display = 'none', 3000);
        }
        
        async function loadSettings() {
            try {
                // Load clock settings
                const clockResp = await fetch('/clock');
                const clock = await clockResp.json();
                
                document.getElementById('enabled').checked = clock.enabled;
                document.getElementById('tune').value = clock.tune;
                document.getElementById('tuneTempo').value = clock.tuneTempo;
                document.getElementById('tuneVelocity').value = clock.tuneVelocity;
                document.getElementById('hourStrike').checked = clock.hourStrike;
                document.getElementById('hourNote1').value = clock.hourNote1;
                document.getElementById('hourNote2').value = clock.hourNote2;
                document.getElementById('hourNote3').value = clock.hourNote3;
                document.getElementById('hourStrikeInterval').value = clock.hourStrikeInterval;
                document.getElementById('hourVelocity').value = clock.hourVelocity;
                document.getElementById('quietModeScale').value = clock.quietModeScale;
                document.getElementById('quietModeStartHour').value = clock.quietModeStartHour;
                document.getElementById('quietModeEndHour').value = clock.quietModeEndHour;
                document.getElementById('silenceStartHour').value = clock.silenceStartHour;
                document.getElementById('silenceEndHour').value = clock.silenceEndHour;
                
                // Load time settings
                const timeResp = await fetch('/time');
                const time = await timeResp.json();
                
                document.getElementById('ntpServer').value = time.ntpServer;
                document.getElementById('timezoneOffset').value = time.timezoneOffset;
                
                showStatus('Settings loaded successfully');
            } catch (error) {
                showStatus('Failed to load settings: ' + error, true);
            }
        }
        
        async function saveAllSettings() {
            try {
                // Save clock settings
                await fetch('/clock/enable?enabled=' + document.getElementById('enabled').checked, {method: 'POST'});
                await fetch('/clock/tune?tune=' + document.getElementById('tune').value, {method: 'POST'});
                await fetch('/clock/tempo?bpm=' + document.getElementById('tuneTempo').value, {method: 'POST'});
                await fetch('/clock/velocity?velocity=' + document.getElementById('tuneVelocity').value, {method: 'POST'});
                await fetch('/clock/hourstrike?enabled=' + document.getElementById('hourStrike').checked, {method: 'POST'});
                await fetch('/clock/hournote?index=0&note=' + document.getElementById('hourNote1').value, {method: 'POST'});
                await fetch('/clock/hournote?index=1&note=' + document.getElementById('hourNote2').value, {method: 'POST'});
                await fetch('/clock/hournote?index=2&note=' + document.getElementById('hourNote3').value, {method: 'POST'});
                await fetch('/clock/strikeinterval?ms=' + document.getElementById('hourStrikeInterval').value, {method: 'POST'});
                await fetch('/clock/hourvelocity?velocity=' + document.getElementById('hourVelocity').value, {method: 'POST'});
                await fetch('/clock/quietscale?scale=' + document.getElementById('quietModeScale').value, {method: 'POST'});
                await fetch('/clock/quiethours?start=' + document.getElementById('quietModeStartHour').value + 
                           '&end=' + document.getElementById('quietModeEndHour').value, {method: 'POST'});
                await fetch('/clock/silencehours?start=' + document.getElementById('silenceStartHour').value + 
                           '&end=' + document.getElementById('silenceEndHour').value, {method: 'POST'});
                
                // Save time settings
                await fetch('/time/timezone?offset=' + document.getElementById('timezoneOffset').value, {method: 'POST'});
                await fetch('/time/ntp?server=' + encodeURIComponent(document.getElementById('ntpServer').value), {method: 'POST'});
                
                showStatus('All settings saved successfully');
            } catch (error) {
                showStatus('Failed to save settings: ' + error, true);
            }
        }
        
        async function syncNTP() {
            try {
                const resp = await fetch('/time/sync');
                const result = await resp.json();
                if (result.success) {
                    showStatus('Time synchronized successfully');
                } else {
                    showStatus('Time sync failed', true);
                }
            } catch (error) {
                showStatus('Failed to sync time: ' + error, true);
            }
        }
        
        async function testChime() {
            try {
                await fetch('/clock/test?quarter=4');
                showStatus('Test chime triggered');
            } catch (error) {
                showStatus('Failed to trigger test chime: ' + error, true);
            }
        }
        
        // Load settings on page load
        window.addEventListener('load', loadSettings);
    </script>
</body>
</html>
)rawliteral";

#endif // SETTINGS_PAGE_H
