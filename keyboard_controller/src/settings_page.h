#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

const char SETTINGS_PAGE_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html><head>
    <title>Keyboard Controller Settings</title>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 0 auto; padding: 20px; background: #f5f5f5; }
        h1 { color: #333; border-bottom: 3px solid #4CAF50; padding-bottom: 10px; }
        h2 { color: #4CAF50; margin-top: 20px; }
        .section { background: white; padding: 20px; margin: 20px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,.1); }
        .setting-row { display: flex; align-items: center; margin: 15px 0; gap: 15px; }
        .setting-row label { flex: 0 0 160px; font-weight: bold; color: #555; }
        .setting-row select { flex: 1; padding: 8px; border: 2px solid #ddd; border-radius: 4px; font-size: 14px; }
        .info-text { color: #666; font-size: 12px; margin-top: 4px; font-style: italic; margin-left: 175px; }
        .button-group { margin-top: 20px; padding-top: 15px; border-top: 1px solid #ddd; }
        button { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; margin: 5px; }
        button:hover { background: #45a049; }
        button.secondary { background: #2196F3; }
        button.secondary:hover { background: #0b7dda; }
        .status { padding: 10px; margin: 10px 0; border-radius: 4px; display: none; }
        .status.success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .status.error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .nav { margin-top: 30px; padding: 15px; background: white; border-radius: 8px; text-align: center; }
        .nav a { color: #4CAF50; text-decoration: none; margin: 0 10px; font-size: 14px; padding: 8px 16px;
                 border: 1px solid #4CAF50; border-radius: 4px; display: inline-block; }
        .nav a:hover { background: #4CAF50; color: white; }
    </style>
</head><body>
    <h1>Keyboard Controller Settings</h1>
    <div id='status' class='status'></div>

    <div class='section'>
        <h2>Hardware Identity</h2>
        <div class='setting-row'>
            <label for='hardwareId'>Controller Type</label>
            <select id='hardwareId'>
                <option value='0'>0 &ndash; Pedal</option>
                <option value='1'>1 &ndash; Great</option>
                <option value='2'>2 &ndash; Swell</option>
                <option value='3'>3 &ndash; Positiv</option>
                <option value='4'>4 &ndash; Echo</option>
                <option value='5'>5 &ndash; Stop-board</option>
            </select>
        </div>
        <div class='info-text'>Determines the CAN output channel and the key-to-MIDI-note mapping for this board. Change takes effect immediately.</div>
        <div class='button-group'>
            <button onclick='saveSettings()'>Save</button>
            <button class='secondary' onclick='loadSettings()'>Refresh</button>
        </div>
    </div>

    <div class='nav'>
        <a href='/'>Home</a>
        <a href='/logs'>Logs</a>
        <a href='/api'>API Docs</a>
    </div>

    <script>
        function showStatus(msg, isError) {
            const s = document.getElementById('status');
            s.textContent = msg;
            s.className = 'status ' + (isError ? 'error' : 'success');
            s.style.display = 'block';
            setTimeout(() => s.style.display = 'none', 3000);
        }
        async function loadSettings() {
            try {
                const resp = await fetch('/config');
                const cfg = await resp.json();
                document.getElementById('hardwareId').value = cfg.hardwareId;
                showStatus('Settings loaded');
            } catch (e) { showStatus('Failed to load: ' + e, true); }
        }
        async function saveSettings() {
            try {
                const id = document.getElementById('hardwareId').value;
                const resp = await fetch('/config/hardware_id?id=' + id, {method: 'POST'});
                if (resp.ok) {
                    showStatus('Saved \u2013 active immediately');
                } else {
                    showStatus('Save failed', true);
                }
            } catch (e) { showStatus('Failed to save: ' + e, true); }
        }
        window.addEventListener('load', loadSettings);
    </script>
</body></html>
)rawliteral";

#endif // SETTINGS_PAGE_H
