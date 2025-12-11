#ifndef API_DOCS_H
#define API_DOCS_H

const char API_DOCS_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Chimes API Documentation</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 1200px;
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
        h3 {
            color: #666;
            margin-top: 20px;
        }
        .endpoint {
            background-color: white;
            border-left: 4px solid #4CAF50;
            padding: 15px;
            margin: 10px 0;
            border-radius: 4px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .method {
            display: inline-block;
            padding: 4px 8px;
            border-radius: 3px;
            font-weight: bold;
            color: white;
            margin-right: 10px;
        }
        .get { background-color: #61affe; }
        .post { background-color: #49cc90; }
        .path {
            font-family: monospace;
            font-size: 16px;
            color: #333;
        }
        .description {
            margin-top: 10px;
            color: #666;
        }
        .params {
            margin-top: 10px;
            background-color: #f9f9f9;
            padding: 10px;
            border-radius: 3px;
        }
        .param {
            font-family: monospace;
            color: #e83e8c;
        }
        .example {
            background-color: #f4f4f4;
            padding: 10px;
            border-radius: 3px;
            margin-top: 10px;
            font-family: monospace;
            font-size: 14px;
        }
        .toc {
            background-color: white;
            padding: 20px;
            border-radius: 4px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .toc a {
            color: #4CAF50;
            text-decoration: none;
        }
        .toc a:hover {
            text-decoration: underline;
        }
        .toc ul {
            list-style-type: none;
            padding-left: 20px;
        }
    </style>
</head>
<body>
    <h1>Chimes Controller API Documentation</h1>
    
    <div class="toc">
        <h3>Table of Contents</h3>
        <ul>
            <li><a href="#clock">Clock Chimes</a></li>
            <li><a href="#time">Time Management</a></li>
            <li><a href="#notes">MIDI Notes</a></li>
            <li><a href="#sequencer">MIDI Sequencer</a></li>
            <li><a href="#repeater">Note Repeater</a></li>
            <li><a href="#songs">Songs</a></li>
            <li><a href="#misc">Miscellaneous</a></li>
        </ul>
    </div>

    <h2 id="clock">Clock Chimes</h2>
    
    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/clock</span>
        <div class="description">Get current clock chime settings and status</div>
        <div class="example">
Response: {
  "enabled": true,
  "tune": 1,
  "tuneName": "Westminster",
  "tuneTempo": 120,
  "tuneVelocity": 127,
  "hourStrike": true,
  "hourNote1": 70,
  "hourNote2": 77,
  "hourNote3": 0,
  "hourStrikeInterval": 2000,
  "hourVelocity": 127,
  "quietModeScale": 64,
  "quietModeStartHour": 22,
  "quietModeEndHour": 7,
  "silenceStartHour": 25,
  "silenceEndHour": 25
}
        </div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/clock/enable</span>
        <div class="description">Enable or disable automatic clock chimes</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">enabled</span> - true/false or 1/0
        </div>
        <div class="example">Example: /clock/enable?enabled=true</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/clock/tune</span>
        <div class="description">Set the chime tune (melody)</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">tune</span> - Tune number (0-5)<br>
            0 = Disabled<br>
            1 = Westminster (Big Ben)<br>
            2 = Whittington<br>
            3 = St. Michael's<br>
            4 = Winchester<br>
            5 = Test Tune (single note)
        </div>
        <div class="example">Example: /clock/tune?tune=1</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/clock/tempo</span>
        <div class="description">Set the chime melody playback tempo</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">bpm</span> - Beats per minute (typically 60-240)
        </div>
        <div class="example">Example: /clock/tempo?bpm=120</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/clock/velocity</span>
        <div class="description">Set the chime melody volume (velocity scaling)</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">velocity</span> - Maximum velocity (1-127, where 127 = full volume)
        </div>
        <div class="example">Example: /clock/velocity?velocity=100</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/clock/hourstrike</span>
        <div class="description">Enable or disable hour striking (bells that count the hour)</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">enabled</span> - true/false or 1/0
        </div>
        <div class="example">Example: /clock/hourstrike?enabled=true</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/clock/hournote</span>
        <div class="description">Set MIDI note for hour strike bells (up to 3 simultaneous notes)</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">index</span> - Note index (0-2)<br>
            <span class="param">note</span> - MIDI note number (69-91, or 255 to disable)
        </div>
        <div class="example">Example: /clock/hournote?index=0&note=70</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/clock/strikeinterval</span>
        <div class="description">Set time between hour strikes</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">ms</span> - Milliseconds between strikes (typically 1000-3000)
        </div>
        <div class="example">Example: /clock/strikeinterval?ms=2000</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/clock/hourvelocity</span>
        <div class="description">Set hour strike volume</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">velocity</span> - Velocity (1-127, where 127 = full volume)
        </div>
        <div class="example">Example: /clock/hourvelocity?velocity=127</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/clock/quietscale</span>
        <div class="description">Set volume scaling during quiet mode hours</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">scale</span> - Velocity scale (0-127, where 64 ≈ 50% volume)
        </div>
        <div class="example">Example: /clock/quietscale?scale=64</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/clock/quiethours</span>
        <div class="description">Set quiet mode hours (reduced volume)</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">start</span> - Start hour (0-24, local time)<br>
            <span class="param">end</span> - End hour (0-24, local time)<br>
            Supports wraparound (e.g., start=22, end=7 means 10 PM to 7 AM)
        </div>
        <div class="example">Example: /clock/quiethours?start=22&end=7</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/clock/silencehours</span>
        <div class="description">Set silence hours (no chimes at all)</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">start</span> - Start hour (0-24, or 25+ to disable)<br>
            <span class="param">end</span> - End hour (0-24, or 25+ to disable)<br>
            Silence mode trumps quiet mode
        </div>
        <div class="example">Example: /clock/silencehours?start=1&end=6</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/clock/test</span>
        <div class="description">Manually trigger a test chime</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">quarter</span> - Quarter to play (1-4)<br>
            1 = Quarter hour (15 min)<br>
            2 = Half hour (30 min)<br>
            3 = Three-quarter hour (45 min)<br>
            4 = Full hour (00 min, includes hour strike if enabled)
        </div>
        <div class="example">Example: /clock/test?quarter=4</div>
    </div>

    <h2 id="time">Time Management</h2>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/time</span>
        <div class="description">Get current time information</div>
        <div class="example">
Response: {
  "timestamp": 1702138245,
  "localTime": "Mon Dec 9 15:30:45 2025",
  "ntpServer": "pool.ntp.org",
  "timezoneOffset": -18000,
  "lastSync": 1702138000
}
        </div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/time/sync</span>
        <div class="description">Manually trigger NTP time synchronization</div>
        <div class="example">Example: /time/sync</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/time/set</span>
        <div class="description">Manually set the system time</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">timestamp</span> - Unix timestamp (seconds since Jan 1, 1970)
        </div>
        <div class="example">Example: /time/set?timestamp=1702138245</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/time/timezone</span>
        <div class="description">Set timezone offset from UTC</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">offset</span> - Offset in seconds from UTC<br>
            Examples:<br>
            UTC-5 (Eastern): -18000<br>
            UTC-6 (Central): -21600<br>
            UTC-7 (Mountain): -25200<br>
            UTC-8 (Pacific): -28800
        </div>
        <div class="example">Example: /time/timezone?offset=-18000</div>
    </div>

    <div class="endpoint">
        <span class="method post">POST</span>
        <span class="path">/time/ntp</span>
        <div class="description">Set NTP server hostname</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">server</span> - NTP server hostname
        </div>
        <div class="example">Example: /time/ntp?server=pool.ntp.org</div>
    </div>

    <h2 id="notes">MIDI Notes</h2>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/note_on</span>
        <div class="description">Trigger a MIDI note on (start playing a note)</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">note</span> - MIDI note number (0-127)<br>
            <span class="param">velocity</span> - Note velocity/volume (1-127, optional, default=100)
        </div>
        <div class="example">Example: /note_on?note=60&velocity=100</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/note_off</span>
        <div class="description">Trigger a MIDI note off (stop playing a note)</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">note</span> - MIDI note number (0-127)
        </div>
        <div class="example">Example: /note_off?note=60</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/all_off</span>
        <div class="description">Turn off all currently playing notes</div>
        <div class="example">Example: /all_off</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/ring/{chime_number}</span>
        <div class="description">Ring a specific chime</div>
        <div class="params">
            <strong>Path Parameter:</strong><br>
            <span class="param">chime_number</span> - Chime number (1-16)
        </div>
        <div class="example">Example: /ring/12</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/ringchannel/{channel_number}</span>
        <div class="description">Ring a specific channel</div>
        <div class="params">
            <strong>Path Parameter:</strong><br>
            <span class="param">channel_number</span> - Channel number (1-16)
        </div>
        <div class="example">Example: /ringchannel/8</div>
    </div>

    <h2 id="sequencer">MIDI Sequencer</h2>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/seq_stop</span>
        <div class="description">Stop the currently playing MIDI sequence</div>
        <div class="example">Example: /seq_stop</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/seq_pause</span>
        <div class="description">Pause the currently playing MIDI sequence</div>
        <div class="example">Example: /seq_pause</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/seq_resume</span>
        <div class="description">Resume a paused MIDI sequence</div>
        <div class="example">Example: /seq_resume</div>
    </div>

    <h2 id="repeater">Note Repeater</h2>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/repeat/start</span>
        <div class="description">Start repeating a note at regular intervals</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">note</span> - MIDI note number (69-91)<br>
            <span class="param">velocity</span> - Note velocity (1-127, optional, default=100)<br>
            <span class="param">period</span> - Milliseconds between repeats (optional, default=1000)<br>
            <span class="param">count</span> - Number of times to repeat (optional, 0 = forever)
        </div>
        <div class="example">Example: /repeat/start?note=70&velocity=127&period=2000&count=12</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/repeat/stop</span>
        <div class="description">Stop repeating a specific note</div>
        <div class="params">
            <strong>Parameters:</strong><br>
            <span class="param">note</span> - MIDI note number to stop repeating
        </div>
        <div class="example">Example: /repeat/stop?note=70</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/repeat/stop_all</span>
        <div class="description">Stop all repeating notes</div>
        <div class="example">Example: /repeat/stop_all</div>
    </div>

    <h2 id="songs">Songs</h2>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/songs</span>
        <div class="description">List all available songs</div>
        <div class="example">Example: /songs</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/play/{song_name}</span>
        <div class="description">Play a song by name</div>
        <div class="params">
            <strong>Path Parameter:</strong><br>
            <span class="param">song_name</span> - Name of the song to play
        </div>
        <div class="example">Example: /play/jingleBells</div>
    </div>

    <h2 id="misc">Miscellaneous</h2>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/</span>
        <div class="description">Main page with basic system information</div>
        <div class="example">Example: /</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/keyboard</span>
        <div class="description">Interactive keyboard interface for playing notes</div>
        <div class="example">Example: /keyboard</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/logs</span>
        <div class="description">View system logs in real-time</div>
        <div class="example">Example: /logs</div>
    </div>

    <div class="endpoint">
        <span class="method get">GET</span>
        <span class="path">/logs/poll</span>
        <div class="description">Poll for new log entries (used by /logs page)</div>
        <div class="example">Example: /logs/poll</div>
    </div>

    <hr>
    
    <h3>Notes:</h3>
    <ul>
        <li>All settings are persisted to NVS (Non-Volatile Storage) and survive reboots and OTA updates</li>
        <li>MIDI note numbers: Middle C (C4) = 60, A4 = 69</li>
        <li>Clock chimes trigger automatically at 15, 30, 45, and 00 minutes past each hour</li>
        <li>Quiet mode scales velocities proportionally during specified hours</li>
        <li>Silence mode completely disables chimes during specified hours</li>
        <li>All time-based settings use local time (based on timezone offset)</li>
    </ul>

    <p style="text-align: center; color: #999; margin-top: 40px;">
        ESP32-S3 Chimes Controller | Built with Arduino Framework
    </p>
</body>
</html>
)rawliteral";

#endif // API_DOCS_H
