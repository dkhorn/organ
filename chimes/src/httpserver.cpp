// httpserver.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "chimes.h"
#include "midinote.h"
#include "keyboard.h"
#include "midiseq.h"
#include "noterepeater.h"
#include "songs.h"
#include "timekeeping.h"
#include "clockchimes.h"
#include "api_docs.h"
#include "settings_page.h"

static WebServer server(80);

// ---------- Simple Circular Log Buffer ----------
#define LOG_BUFFER_SIZE 100
static String logBuffer[LOG_BUFFER_SIZE];
static int logIndex = 0;
static int logCount = 0;
static int logTotalCount = 0; // Monotonically increasing counter
static bool httpLoggingEnabled = false; // Only enable when logs requested
static String pendingLogLine = ""; // Accumulator for incomplete lines

void httpserver_log(const uint8_t* buffer, size_t size) {
  if (!httpLoggingEnabled) return; // Skip all String building if disabled
  
  // Build String from buffer, accumulating partial lines
  for (size_t i = 0; i < size; i++) {
    char c = buffer[i];
    if (c == '\n') {
      if (pendingLogLine.length() > 0) {
        // Store complete line in buffer
        logBuffer[logIndex] = pendingLogLine;
        logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
        if (logCount < LOG_BUFFER_SIZE) logCount++;
        logTotalCount++;
        pendingLogLine = "";
      }
    } else if (c != '\r') {
      pendingLogLine += c;
    }
  }
}

// Legacy C-string interface (for compatibility)
void httpserver_log(const char* message) {
  if (!httpLoggingEnabled) return;
  
  logBuffer[logIndex] = String(message);
  logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
  if (logCount < LOG_BUFFER_SIZE) logCount++;
  logTotalCount++;
}

String httpserver_get_logs() {
  String result = "";
  int start = (logCount < LOG_BUFFER_SIZE) ? 0 : logIndex;
  for (int i = 0; i < logCount; i++) {
    int idx = (start + i) % LOG_BUFFER_SIZE;
    result += logBuffer[idx] + "\n";
  }
  return result;
}

String httpserver_get_logs_since(int sinceTotal) {
  String result = "";
  
  // If asking for logs before our buffer, or invalid, return all
  if (sinceTotal < 0) {
    return httpserver_get_logs();
  }
  
  // Calculate how many new logs since the requested point
  int newCount = logTotalCount - sinceTotal;
  
  // If no new logs, return empty
  if (newCount <= 0) {
    return "";
  }
  
  // If asking for more logs than we have buffered, return all
  if (newCount > logCount) {
    return httpserver_get_logs();
  }
  
  // Calculate starting position in circular buffer
  int start = (logIndex - newCount + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
  for (int i = 0; i < newCount; i++) {
    int idx = (start + i) % LOG_BUFFER_SIZE;
    result += logBuffer[idx] + "\n";
  }
  return result;
}

int httpserver_get_log_index() {
  return logTotalCount;
}

// Handler for GET /channels
static void handleChannels() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += "h1 { color: #333; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }";
  html += ".controls { display: flex; gap: 15px; margin: 15px 0; flex-wrap: wrap; align-items: center; }";
  html += ".control-group { display: flex; gap: 5px; align-items: center; }";
  html += ".control-group label { font-weight: bold; }";
  html += ".control-group input { width: 80px; padding: 5px; font-size: 14px; border: 2px solid #ccc; border-radius: 4px; }";
  html += ".button-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(80px, 1fr)); gap: 10px; margin-top: 20px; }";
  html += "button { padding: 15px; font-size: 16px; border: 2px solid #4CAF50; background: #4CAF50; color: white; border-radius: 5px; cursor: pointer; transition: all 0.3s; }";
  html += "button:hover { background: #45a049; transform: scale(1.05); }";
  html += "button:active { transform: scale(0.95); }";
  html += ".status { margin-top: 15px; padding: 10px; background: #e8f5e9; border-radius: 5px; display: none; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Chime Controller</h1>";
  html += "<p>Click a button to ring a chime by channel (0-20)</p>";
  html += "<div class='controls'>";
  html += "<div class='control-group'>";
  html += "<label for='kickDuty'>Kick Duty:</label>";
  html += "<input type='number' id='kickDuty' value='100' min='1' max='100' />";
  html += "<span>%</span>";
  html += "</div>";
  html += "<div class='control-group'>";
  html += "<label for='kickHold'>Kick Hold:</label>";
  html += "<input type='number' id='kickHold' value='35' min='0' max='1000' />";
  html += "<span>ms</span>";
  html += "</div>";
  html += "</div>";
  html += "<div class='button-grid'>";
  
  // Create 21 buttons (notes 0-20)
  for (int i = 0; i < 21; i++) {
    html += "<button onclick='ring(" + String(i) + ")'>Ch " + String(i) + "</button>";
  }
  
  html += "</div>";
  html += "<div class='status' id='status'></div>";
  html += "</div>";
  
  // JavaScript to handle button clicks
  html += "<script>";
  html += "function ring(note) {";
  html += "  const duty = document.getElementById('kickDuty').value;";
  html += "  const hold = document.getElementById('kickHold').value;";
  html += "  fetch('/ringchannel/' + note + '?duty=' + duty + '&hold=' + hold)";
  html += "    .then(r => r.text())";
  html += "    .then(msg => {";
  html += "      const s = document.getElementById('status');";
  html += "      s.textContent = msg;";
  html += "      s.style.display = 'block';";
  html += "      setTimeout(() => s.style.display = 'none', 2000);";
  html += "    })";
  html += "    .catch(e => console.error('Error:', e));";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Handler for GET /ring/<note>
static void handleRing() {
  String path = server.uri();
  
  // Path should be like "/ring/5"
  if (!path.startsWith("/ring/")) {
    server.send(400, "text/plain", "Bad Request: Invalid path");
    return;
  }
  
  String noteStr = path.substring(6); // Skip "/ring/"
  int note = noteStr.toInt();
  
  if (note < 0 || note >= 21) {
    server.send(400, "text/plain", "Bad Request: Note must be 0-20");
    return;
  }
  
  ring_chime(note, 127);
  
  String response = "Rang note " + String(note);
  server.send(200, "text/plain", response);
}

// Handler for GET /ringchannel/<channel>?duty=<duty>&hold=<hold>
static void handleRingChannel() {
  String path = server.uri();
  
  // Path should be like "/ringchannel/5"
  if (!path.startsWith("/ringchannel/")) {
    server.send(400, "text/plain", "Bad Request: Invalid path");
    return;
  }
  
  String channelStr = path.substring(13); // Skip "/ringchannel/"
  int channel = channelStr.toInt();
  
  if (channel < 0 || channel >= 21) {
    server.send(400, "text/plain", "Bad Request: Channel must be 0-20");
    return;
  }
  
  // Parse optional query parameters
  int duty = 100;
  int hold = 35;
  
  if (server.hasArg("duty")) {
    duty = server.arg("duty").toInt();
    if (duty < 1) duty = 1;
    if (duty > 100) duty = 100;
  }
  
  if (server.hasArg("hold")) {
    hold = server.arg("hold").toInt();
    if (hold < 0) hold = 0;
    if (hold > 1000) hold = 1000;
  }
  
  // Ring the physical channel with custom parameters
  ring_chime_raw(channel, duty, hold);
  
  String response = "Rang channel " + String(channel) + " (duty=" + String(duty) + "%, hold=" + String(hold) + "ms)";
  server.send(200, "text/plain", response);
}

// Handler for GET /logs - Log viewer page
static void handleLogsPage() {
  httpLoggingEnabled = true; // Enable logging on first access
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Chime Logs</title>";
  html += "<style>";
  html += "body { font-family: monospace; margin: 0; padding: 20px; background: #1e1e1e; color: #d4d4d4; }";
  html += "h1 { color: #4ec9b0; margin-bottom: 10px; }";
  html += "#log { background: #252526; border: 1px solid #3e3e42; padding: 15px; ";
  html += "height: 70vh; overflow-y: auto; white-space: pre-wrap; word-wrap: break-word; }";
  html += ".controls { margin-bottom: 15px; }";
  html += "button { padding: 8px 16px; background: #0e639c; color: white; border: none; ";
  html += "border-radius: 4px; cursor: pointer; margin-right: 10px; }";
  html += "button:hover { background: #1177bb; }";
  html += ".status { color: #4ec9b0; margin-left: 10px; }";
  html += "</style></head><body>";
  html += "<h1>Chime Controller Logs</h1>";
  html += "<div class='controls'>";
  html += "<button onclick='clearLog()'>Clear</button>";
  html += "<button onclick='toggleAutoScroll()' id='scrollBtn'>Auto-scroll: ON</button>";
  html += "<span class='status' id='status'>Connected</span>";
  html += "</div>";
  html += "<div id='log'></div>";
  html += "<script>";
  html += "let autoScroll = true;";
  html += "let lastIndex = -1;";
  html += "const logDiv = document.getElementById('log');";
  html += "const statusDiv = document.getElementById('status');";
  html += "function fetchLogs() {";
  html += "  fetch('/logs/poll?since=' + lastIndex)";
  html += "    .then(r => r.json())";
  html += "    .then(data => {";
  html += "      if (data.logs) {";
  html += "        logDiv.textContent += data.logs;";
  html += "        if (autoScroll) logDiv.scrollTop = logDiv.scrollHeight;";
  html += "      }";
  html += "      lastIndex = data.index;";
  html += "      statusDiv.textContent = 'Connected';";
  html += "      statusDiv.style.color = '#4ec9b0';";
  html += "    })";
  html += "    .catch(() => {";
  html += "      statusDiv.textContent = 'Error';";
  html += "      statusDiv.style.color = '#f48771';";
  html += "    });";
  html += "}";
  html += "fetchLogs();";
  html += "setInterval(fetchLogs, 1000);";
  html += "function clearLog() { logDiv.textContent = ''; }";
  html += "function toggleAutoScroll() {";
  html += "  autoScroll = !autoScroll;";
  html += "  document.getElementById('scrollBtn').textContent = 'Auto-scroll: ' + (autoScroll ? 'ON' : 'OFF');";
  html += "}";
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}

// Handler for GET /logs/poll?since=X - Polling endpoint
static void handleLogsPoll() {
  httpLoggingEnabled = true; // Enable logging if not already
  
  int sinceIndex = -1;
  if (server.hasArg("since")) {
    sinceIndex = server.arg("since").toInt();
  }
  
  String logs = httpserver_get_logs_since(sinceIndex);
  int currentIndex = httpserver_get_log_index();
  
  // Early return for no new logs
  if (logs.length() == 0) {
    String json = "{\"index\":" + String(currentIndex) + ",\"logs\":\"\"}";
    server.send(200, "application/json", json);
    return;
  }
  
  // Reserve capacity to minimize reallocations (rough estimate: logs.length() * 1.2 + 50)
  String json;
  json.reserve(logs.length() + logs.length() / 5 + 50);
  
  json = "{\"index\":" + String(currentIndex) + ",\"logs\":\"";
  
  // Escape the logs for JSON
  for (unsigned int i = 0; i < logs.length(); i++) {
    char c = logs[i];
    if (c == '"') json += "\\\"";
    else if (c == '\\') json += "\\\\";
    else if (c == '\n') json += "\\n";
    else if (c == '\r') json += "\\r";
    else if (c == '\t') json += "\\t";
    else json += c;
  }
  json += "\"}";
  
  server.send(200, "application/json", json);
}

// Handler for GET /keyboard
// Handler for GET /
static void handleRoot() {
  server.send(200, "text/html", get_keyboard_html());
}

// Handler for GET /note_on?note=X&velocity=Y
static void handleNoteOn() {
  if (!server.hasArg("note")) {
    server.send(400, "text/plain", "Bad Request: Missing 'note' parameter");
    return;
  }
  
  int note = server.arg("note").toInt();
  int velocity = server.hasArg("velocity") ? server.arg("velocity").toInt() : 127;
  
  if (note < 0 || note > 127) {
    server.send(400, "text/plain", "Bad Request: Note must be 0-127");
    return;
  }
  
  if (velocity < 0 || velocity > 127) {
    server.send(400, "text/plain", "Bad Request: Velocity must be 0-127");
    return;
  }
  
  note_on((uint8_t)note, (uint8_t)velocity);
  
  String response = "Note On: " + String(note) + " velocity: " + String(velocity);
  server.send(200, "text/plain", response);
}

// Handler for GET /note_off?note=X&velocity=Y
static void handleNoteOff() {
  if (!server.hasArg("note")) {
    server.send(400, "text/plain", "Bad Request: Missing 'note' parameter");
    return;
  }
  
  int note = server.arg("note").toInt();
  int velocity = server.hasArg("velocity") ? server.arg("velocity").toInt() : 64;
  
  if (note < 0 || note > 127) {
    server.send(400, "text/plain", "Bad Request: Note must be 0-127");
    return;
  }
  
  if (velocity < 0 || velocity > 127) {
    server.send(400, "text/plain", "Bad Request: Velocity must be 0-127");
    return;
  }
  
  note_off((uint8_t)note, (uint8_t)velocity);
  
  String response = "Note Off: " + String(note) + " velocity: " + String(velocity);
  server.send(200, "text/plain", response);
}

// Handle /all_off endpoint - panic button
void handleAllOff() {
  midiseq_stop();
  all_off();
  // Serial.println("All notes off (panic)");
  server.send(200, "text/plain", "All notes off");
}

// Handler for GET /repeat/start?note=X&velocity=Y&period=Z&count=N
// Start a repeated note. Count=0 means repeat forever.
static void handleRepeatStart() {
  if (!server.hasArg("note")) {
    server.send(400, "text/plain", "Bad Request: Missing 'note' parameter");
    return;
  }
  if (!server.hasArg("period")) {
    server.send(400, "text/plain", "Bad Request: Missing 'period' parameter (ms)");
    return;
  }
  
  int note = server.arg("note").toInt();
  int velocity = server.hasArg("velocity") ? server.arg("velocity").toInt() : 100;
  int period_ms = server.arg("period").toInt();
  int count = server.hasArg("count") ? server.arg("count").toInt() : 0;
  
  if (note < 0 || note > 127) {
    server.send(400, "text/plain", "Bad Request: Note must be 0-127");
    return;
  }
  
  if (velocity < 1 || velocity > 127) {
    server.send(400, "text/plain", "Bad Request: Velocity must be 1-127");
    return;
  }
  
  if (period_ms < 100) {
    server.send(400, "text/plain", "Bad Request: Period must be >= 100ms");
    return;
  }
  
  if (count < 0) {
    server.send(400, "text/plain", "Bad Request: Count must be >= 0 (0=forever)");
    return;
  }
  
  start_repeated_note((uint8_t)note, (uint8_t)velocity, (uint32_t)period_ms, (uint16_t)count);
  
  String response = "Started repeating note " + String(note) + 
                    " velocity=" + String(velocity) + 
                    " period=" + String(period_ms) + "ms" +
                    " count=" + String(count == 0 ? "forever" : String(count));
  server.send(200, "text/plain", response);
}

// Handler for GET /repeat/stop?note=X
// Stop a specific repeated note
static void handleRepeatStop() {
  if (!server.hasArg("note")) {
    server.send(400, "text/plain", "Bad Request: Missing 'note' parameter");
    return;
  }
  
  int note = server.arg("note").toInt();
  
  if (note < 0 || note > 127) {
    server.send(400, "text/plain", "Bad Request: Note must be 0-127");
    return;
  }
  
  stop_repeated_note((uint8_t)note);
  
  String response = "Stopped repeating note " + String(note);
  server.send(200, "text/plain", response);
}

// Handler for GET /repeat/stop_all
// Stop all repeated notes
static void handleRepeatStopAll() {
  stop_all_repeated_notes();
  server.send(200, "text/plain", "Stopped all repeating notes");
}


// Handle /play/<song_id> endpoint - play a song from the database
// Optional query parameters: transpose (semitones), tempo (BPM)
// Example: /play/0?transpose=5&tempo=140
static void handlePlaySong() {
  String uri = server.uri();
  
  // Extract song ID from /play/<id>
  int song_id = uri.substring(6).toInt();  // Skip "/play/"
  
  if (song_id < 0 || song_id >= NUM_SONGS) {
    server.send(400, "text/plain", "Invalid song ID. Valid range: 0-" + String(NUM_SONGS - 1));
    return;
  }
  
  // Parse optional query parameters
  int8_t transpose = 0;
  if (server.hasArg("transpose")) {
    transpose = server.arg("transpose").toInt();
    // Clamp to reasonable range
    if (transpose < -24) transpose = -24;
    if (transpose > 24) transpose = 24;
  }
  
  const Song* song = &SONGS[song_id];
  uint16_t tempo = song->default_tempo_bpm;
  if (server.hasArg("tempo")) {
    tempo = server.arg("tempo").toInt();
    // Clamp to reasonable range
    if (tempo < 20) tempo = 20;
    if (tempo > 300) tempo = 300;
  }
  
  midiseq_load(song->events, song->num_events, song->ticks_per_quarter, tempo, transpose);
  midiseq_play();
  
  String response = "Playing: " + String(song->name);
  if (transpose != 0) {
    response += " (transpose " + String(transpose) + " semitones)";
  }
  if (tempo != song->default_tempo_bpm) {
    response += " (" + String(tempo) + " BPM)";
  } else {
    response += " (" + String(tempo) + " BPM)";
  }
  server.send(200, "text/plain", response);
  // Serial.println(response);
}

// Handle /songs endpoint - list all available songs
static void handleListSongs() {
  String response = "Available songs:\n\n";
  
  for (int i = 0; i < NUM_SONGS; i++) {
    response += String(i) + ": " + String(SONGS[i].name);
    response += " (" + String(SONGS[i].num_events) + " events, ";
    response += String(SONGS[i].default_tempo_bpm) + " BPM)\n";
    response += "   Play: /play/" + String(i) + "\n\n";
  }
  
  server.send(200, "text/plain", response);
}

// Handle /seq_stop endpoint
static void handleSeqStop() {
  midiseq_stop();
  server.send(200, "text/plain", "Sequence stopped");
  // Serial.println("Sequence stopped");
}

// Handle /seq_pause endpoint
static void handleSeqPause() {
  midiseq_pause();
  server.send(200, "text/plain", "Sequence paused");
}

// Handle /seq_resume endpoint
static void handleSeqResume() {
  midiseq_resume();
  server.send(200, "text/plain", "Sequence resumed");
}

// Handler for 404 Not Found
static void handleNotFound() {
  String message = "Not Found\n\n";
  message += "URI: " + server.uri() + "\n";
  message += "Method: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n";
  server.send(404, "text/plain", message);
}

// Handler for GET /clock
static void handleClockStatus() {
  String json = "{";
  json += "\"enabled\":" + String(clockChimes.isEnabled() ? "true" : "false") + ",";
  json += "\"tune\":" + String(clockChimes.getTune()) + ",";
  json += "\"tuneName\":\"" + String(clockChimes.getTuneName(clockChimes.getTune())) + "\",";
  json += "\"tuneTempo\":" + String(clockChimes.getTuneTempo()) + ",";
  json += "\"tuneVelocity\":" + String(clockChimes.getTuneVelocity()) + ",";
  json += "\"hourStrike\":" + String(clockChimes.isHourStrikeEnabled() ? "true" : "false") + ",";
  json += "\"hourNote1\":" + String(clockChimes.getHourNote(0)) + ",";
  json += "\"hourNote2\":" + String(clockChimes.getHourNote(1)) + ",";
  json += "\"hourNote3\":" + String(clockChimes.getHourNote(2)) + ",";
  json += "\"hourStrikeInterval\":" + String(clockChimes.getHourStrikeInterval()) + ",";
  json += "\"hourVelocity\":" + String(clockChimes.getHourVelocity()) + ",";
  json += "\"quietModeScale\":" + String(clockChimes.getQuietModeScale()) + ",";
  json += "\"quietModeStartHour\":" + String(clockChimes.getQuietModeStartHour()) + ",";
  json += "\"quietModeEndHour\":" + String(clockChimes.getQuietModeEndHour()) + ",";
  json += "\"silenceStartHour\":" + String(clockChimes.getSilenceStartHour()) + ",";
  json += "\"silenceEndHour\":" + String(clockChimes.getSilenceEndHour());
  json += "}";
  
  server.send(200, "application/json", json);
}

// Handler for POST /clock/enable
static void handleClockEnable() {
  if (!server.hasArg("enabled")) {
    server.send(400, "text/plain", "Missing enabled parameter (true/false)");
    return;
  }
  
  bool enabled = server.arg("enabled") == "true" || server.arg("enabled") == "1";
  clockChimes.setEnabled(enabled);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /clock/tune
static void handleClockTune() {
  if (!server.hasArg("tune")) {
    server.send(400, "text/plain", "Missing tune parameter (0-4)");
    return;
  }
  
  uint8_t tune = server.arg("tune").toInt();
  clockChimes.setTune(tune);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /clock/hourstrike
static void handleClockHourStrike() {
  if (!server.hasArg("enabled")) {
    server.send(400, "text/plain", "Missing enabled parameter (true/false)");
    return;
  }
  
  bool enabled = server.arg("enabled") == "true" || server.arg("enabled") == "1";
  clockChimes.setHourStrikeEnabled(enabled);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /clock/hournote
static void handleClockHourNote() {
  if (!server.hasArg("index") || !server.hasArg("note")) {
    server.send(400, "text/plain", "Missing index (0-2) or note (0-127, 255=disable) parameter");
    return;
  }
  
  uint8_t index = server.arg("index").toInt();
  uint8_t note = server.arg("note").toInt();
  clockChimes.setHourNote(index, note);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /clock/tempo
static void handleClockTempo() {
  if (!server.hasArg("bpm")) {
    server.send(400, "text/plain", "Missing bpm parameter (beats per minute)");
    return;
  }
  
  uint16_t bpm = server.arg("bpm").toInt();
  clockChimes.setTuneTempo(bpm);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /clock/velocity
static void handleClockVelocity() {
  if (!server.hasArg("velocity")) {
    server.send(400, "text/plain", "Missing velocity parameter (1-127)");
    return;
  }
  
  uint8_t velocity = server.arg("velocity").toInt();
  clockChimes.setTuneVelocity(velocity);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /clock/strikeinterval
static void handleClockStrikeInterval() {
  if (!server.hasArg("ms")) {
    server.send(400, "text/plain", "Missing ms parameter (milliseconds)");
    return;
  }
  
  uint32_t intervalMs = server.arg("ms").toInt();
  clockChimes.setHourStrikeInterval(intervalMs);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /clock/hourvelocity
static void handleHourVelocity() {
  if (!server.hasArg("velocity")) {
    server.send(400, "text/plain", "Missing velocity parameter (1-127)");
    return;
  }
  
  uint8_t velocity = server.arg("velocity").toInt();
  clockChimes.setHourVelocity(velocity);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /clock/quietscale
static void handleQuietScale() {
  if (!server.hasArg("scale")) {
    server.send(400, "text/plain", "Missing scale parameter (0-127)");
    return;
  }
  
  uint8_t scale = server.arg("scale").toInt();
  clockChimes.setQuietModeScale(scale);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /clock/quiethours
static void handleQuietHours() {
  if (!server.hasArg("start") || !server.hasArg("end")) {
    server.send(400, "text/plain", "Missing start or end parameter (0-24)");
    return;
  }
  
  uint8_t start = server.arg("start").toInt();
  uint8_t end = server.arg("end").toInt();
  clockChimes.setQuietModeStartHour(start);
  clockChimes.setQuietModeEndHour(end);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /clock/silencehours
static void handleSilenceHours() {
  if (!server.hasArg("start") || !server.hasArg("end")) {
    server.send(400, "text/plain", "Missing start or end parameter (0-24, 25+=disabled)");
    return;
  }
  
  uint8_t start = server.arg("start").toInt();
  uint8_t end = server.arg("end").toInt();
  clockChimes.setSilenceStartHour(start);
  clockChimes.setSilenceEndHour(end);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for GET /clock/test
static void handleClockTest() {
  if (!server.hasArg("quarter")) {
    server.send(400, "text/plain", "Missing quarter parameter (1-4)");
    return;
  }
  
  uint8_t quarter = server.arg("quarter").toInt();
  clockChimes.manualChime(quarter);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for GET /time
static void handleTime() {
  char timeStr[64];
  timekeeping.getTimeString(timeStr, sizeof(timeStr));
  
  String json = "{";
  json += "\"timestamp\":" + String(timekeeping.getTimestamp()) + ",";
  json += "\"localTime\":\"" + String(timeStr) + "\",";
  json += "\"synced\":" + String(timekeeping.isSynced() ? "true" : "false") + ",";
  json += "\"lastSync\":" + String(timekeeping.getLastSyncTime()) + ",";
  json += "\"timezoneOffset\":" + String(timekeeping.getTimezoneOffset());
  json += "}";
  
  server.send(200, "application/json", json);
}

// Handler for GET /time/sync
static void handleTimeSync() {
  bool success = timekeeping.syncNTP();
  String json = "{\"success\":" + String(success ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

// Handler for POST /time/set
static void handleTimeSet() {
  if (!server.hasArg("timestamp")) {
    server.send(400, "text/plain", "Missing timestamp parameter");
    return;
  }
  
  time_t timestamp = server.arg("timestamp").toInt();
  timekeeping.setTime(timestamp);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /time/timezone
static void handleTimeZone() {
  if (!server.hasArg("offset")) {
    server.send(400, "text/plain", "Missing offset parameter (seconds from UTC)");
    return;
  }
  
  long offset = server.arg("offset").toInt();
  timekeeping.setTimezoneOffset(offset);
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for POST /time/ntp
static void handleTimeNTPServer() {
  if (!server.hasArg("server")) {
    server.send(400, "text/plain", "Missing server parameter");
    return;
  }
  
  String ntpServer = server.arg("server");
  timekeeping.setNTPServer(ntpServer.c_str());
  server.send(200, "application/json", "{\"success\":true}");
}

// Handler for GET /api
static void handleAPIDocumentation() {
  server.send(200, "text/html", API_DOCS_HTML);
}

// Handler for GET /settings
static void handleSettings() {
  server.send(200, "text/html", SETTINGS_PAGE_HTML);
}

extern "C" {

void httpserver_begin() {
  // Register route handlers - use onNotFound pattern matching
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api", HTTP_GET, handleAPIDocumentation);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/channels", HTTP_GET, handleChannels);
  server.on("/logs", HTTP_GET, handleLogsPage);
  server.on("/logs/poll", HTTP_GET, handleLogsPoll);
  server.on("/note_on", HTTP_GET, handleNoteOn);
  server.on("/note_off", HTTP_GET, handleNoteOff);
  server.on("/all_off", HTTP_GET, handleAllOff);
  server.on("/repeat/start", HTTP_GET, handleRepeatStart);
  server.on("/repeat/stop", HTTP_GET, handleRepeatStop);
  server.on("/repeat/stop_all", HTTP_GET, handleRepeatStopAll);
  server.on("/songs", HTTP_GET, handleListSongs);
  server.on("/seq_stop", HTTP_GET, handleSeqStop);
  server.on("/seq_pause", HTTP_GET, handleSeqPause);
  server.on("/seq_resume", HTTP_GET, handleSeqResume);
  server.on("/clock", HTTP_GET, handleClockStatus);
  server.on("/clock/enable", HTTP_POST, handleClockEnable);
  server.on("/clock/tune", HTTP_POST, handleClockTune);
  server.on("/clock/hourstrike", HTTP_POST, handleClockHourStrike);
  server.on("/clock/hournote", HTTP_POST, handleClockHourNote);
  server.on("/clock/tempo", HTTP_POST, handleClockTempo);
  server.on("/clock/velocity", HTTP_POST, handleClockVelocity);
  server.on("/clock/strikeinterval", HTTP_POST, handleClockStrikeInterval);
  server.on("/clock/hourvelocity", HTTP_POST, handleHourVelocity);
  server.on("/clock/quietscale", HTTP_POST, handleQuietScale);
  server.on("/clock/quiethours", HTTP_POST, handleQuietHours);
  server.on("/clock/silencehours", HTTP_POST, handleSilenceHours);
  server.on("/clock/test", HTTP_GET, handleClockTest);
  server.on("/time", HTTP_GET, handleTime);
  server.on("/time/sync", HTTP_GET, handleTimeSync);
  server.on("/time/set", HTTP_POST, handleTimeSet);
  server.on("/time/timezone", HTTP_POST, handleTimeZone);
  server.on("/time/ntp", HTTP_POST, handleTimeNTPServer);
  
  // For parameterized routes, we'll handle them in onNotFound
  // and check the path prefix there
  server.onNotFound([]() {
    String uri = server.uri();
    
    if (uri.startsWith("/ring/")) {
      handleRing();
    } else if (uri.startsWith("/ringchannel/")) {
      handleRingChannel();
    } else if (uri.startsWith("/play/")) {
      handlePlaySong();
    } else {
      handleNotFound();
    }
  });
  
  // Start server
  server.begin();
  // Serial.println("HTTP server started on port 80");
}

void httpserver_loop() {
  server.handleClient();
}

} // extern "C"
