// httpserver.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
// #include "chimes.h"
#include "output.h"
#include "midinote.h"
#include "keyboard.h"
#include "midiseq.h"
// #include "noterepeater.h"
// #include "songs.h"
#include "timekeeping.h"
// #include "clockchimes.h"
#include "midiudp.h"
// #include "midifiles.h"
#include "api_docs.h"
#include "settings_page.h"

static WebServer server(80);

// ---------- Simple Circular Log Buffer ----------
#define LOG_BUFFER_SIZE 100
static String logBuffer[LOG_BUFFER_SIZE];
static int logIndex = 0;
static int logCount = 0;
static int logTotalCount = 0; // Monotonically increasing counter
static bool httpLoggingEnabled = true; // Only enable when logs requested
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
  html += "  fetch('/note_on_by_index' + '?note=' + note)";
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

// Handler for GET /note_on_by_indx?note=X
static void handleNoteOnByIndex() {
  if (!server.hasArg("note")) {
    server.send(400, "text/plain", "Bad Request: Missing 'note' parameter");
    return;
  }
  
  int note = server.arg("note").toInt();

  setChannel(note, true);
  flushOutput();
  
  String response = "Note On: " + String(note);
  server.send(200, "text/plain", response);
}

// Handler for GET /note_off_by_indx?note=X
static void handleNoteOffByIndex() {
  if (!server.hasArg("note")) {
    server.send(400, "text/plain", "Bad Request: Missing 'note' parameter");
    return;
  }
  
  int note = server.arg("note").toInt();

  setChannel(note, false);
  flushOutput();
  
  String response = "Note Off: " + String(note);
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

// Handler for GET /status - system status including MIDI/UDP
static void handleStatus() {
  String json = "{";
  json += "\"uptime\":" + String(millis()) + ",";
  json += "\"wifi\":{";
  json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI());
  json += "},";
  json += "\"midiUdp\":{";
  json += "\"listening\":" + String(midiUDP.isListening() ? "true" : "false") + ",";
  json += "\"port\":" + String(midiUDP.getPort()) + ",";
  json += "\"packetsReceived\":" + String(midiUDP.getPacketsReceived()) + ",";
  json += "\"messagesReceived\":" + String(midiUDP.getMessagesReceived()) + ",";
  json += "\"packetsDropped\":" + String(midiUDP.getPacketsDropped());
  json += "},";
  json += "\"time\":{";
  json += "\"synced\":" + String(timekeeping.isSynced() ? "true" : "false") + ",";
  json += "\"timestamp\":" + String(timekeeping.getTimestamp());
  json += "}";
  json += "}";
  
  server.send(200, "application/json", json);
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

// Handler for GET /player - MIDI file player UI
static void handlePlayer() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>MIDI Player</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += "h1 { color: #333; }";
  html += ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }";
  html += ".file { display: flex; justify-content: space-between; align-items: center; padding: 15px; margin: 10px 0; background: #f9f9f9; border-radius: 5px; border-left: 4px solid #4CAF50; }";
  html += ".file-info { flex-grow: 1; }";
  html += ".file-name { font-weight: bold; font-size: 16px; }";
  html += ".file-size { color: #666; font-size: 14px; }";
  html += ".controls { display: flex; gap: 10px; align-items: center; }";
  html += "button { padding: 8px 16px; font-size: 14px; border: none; background: #4CAF50; color: white; border-radius: 4px; cursor: pointer; }";
  html += "button:hover { background: #45a049; }";
  html += "button.delete { background: #f44336; }";
  html += "button.delete:hover { background: #da190b; }";
  html += "input[type=number] { width: 60px; padding: 5px; border: 2px solid #ddd; border-radius: 4px; }";
  html += ".upload { margin-bottom: 20px; padding: 15px; background: #e3f2fd; border-radius: 5px; }";
  html += ".status { margin-top: 15px; padding: 10px; background: #e8f5e9; border-radius: 5px; display: none; }";
  html += ".error { background: #ffebee !important; color: #c62828; }";
  html += ".storage { margin-top: 15px; padding: 10px; background: #fff3e0; border-radius: 5px; font-size: 14px; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>MIDI File Player</h1>";
  html += "<div class='upload'>";
  html += "<input type='file' id='fileInput' accept='.mid,.midi' />";
  html += "<button onclick='uploadFile()'>Upload</button>";
  html += "</div>";
  html += "<div id='storage' class='storage'>Loading storage info...</div>";
  html += "<div id='fileList'>Loading files...</div>";
  html += "<div id='status' class='status'></div>";
  html += "</div>";
  html += "<script>";
  html += "function loadFiles() {";
  html += "  fetch('/files').then(r => r.json()).then(data => {";
  html += "    let html = '';";
  html += "    if (data.files.length === 0) {";
  html += "      html = '<p>No MIDI files uploaded yet.</p>';";
  html += "    } else {";
  html += "      data.files.forEach(f => {";
  html += "        html += '<div class=\"file\">';";
  html += "        html += '<div class=\"file-info\"><div class=\"file-name\">' + f.name + '</div>';";
  html += "        html += '<div class=\"file-size\">' + f.size + ' bytes</div></div>';";
  html += "        html += '<div class=\"controls\">';";
  html += "        html += '<label>Vel:</label><input type=\"number\" id=\"vel_' + f.name + '\" value=\"1.0\" min=\"0\" max=\"2\" step=\"0.1\" />';";
  html += "        html += '<label>Tempo:</label><input type=\"number\" id=\"tempo_' + f.name + '\" value=\"1.0\" min=\"0.1\" max=\"4\" step=\"0.1\" />';";
  html += "        html += '<label>Transpose:</label><input type=\"number\" id=\"trans_' + f.name + '\" value=\"0\" min=\"-12\" max=\"12\" />';";
  html += "        html += '<button onclick=\"playFile(\\'' + f.name + '\\');\">Play</button>';";
  html += "        html += '<button class=\"delete\" onclick=\"deleteFile(\\'' + f.name + '\\');\">Delete</button>';";
  html += "        html += '</div></div>';";
  html += "      });";
  html += "    }";
  html += "    document.getElementById('fileList').innerHTML = html;";
  html += "    const st = data.storage;";
  html += "    document.getElementById('storage').textContent = 'Storage: ' + (st.used/1024).toFixed(1) + ' KB used / ' + (st.total/1024).toFixed(1) + ' KB total (' + (st.free/1024).toFixed(1) + ' KB free)';";
  html += "  });";
  html += "}";
  html += "function playFile(name) {";
  html += "  const vel = document.getElementById('vel_' + name).value;";
  html += "  const tempo = document.getElementById('tempo_' + name).value;";
  html += "  const trans = document.getElementById('trans_' + name).value;";
  html += "  fetch('/files/play?name=' + encodeURIComponent(name) + '&velocity=' + vel + '&tempo=' + tempo + '&transpose=' + trans, {method: 'POST'})";
  html += "    .then(r => r.json()).then(d => showStatus(d.message, d.success));";
  html += "}";
  html += "function deleteFile(name) {";
  html += "  if (!confirm('Delete ' + name + '?')) return;";
  html += "  fetch('/files/' + encodeURIComponent(name), {method: 'DELETE'})";
  html += "    .then(r => r.json()).then(d => { showStatus(d.message, d.success); loadFiles(); });";
  html += "}";
  html += "function uploadFile() {";
  html += "  const input = document.getElementById('fileInput');";
  html += "  if (!input.files[0]) { alert('Select a file first'); return; }";
  html += "  const formData = new FormData();";
  html += "  formData.append('file', input.files[0]);";
  html += "  fetch('/files/upload', {method: 'POST', body: formData})";
  html += "    .then(r => r.json()).then(d => { showStatus(d.message, d.success); if(d.success) loadFiles(); });";
  html += "}";
  html += "function showStatus(msg, ok) {";
  html += "  const s = document.getElementById('status');";
  html += "  s.textContent = msg;";
  html += "  s.className = ok ? 'status' : 'status error';";
  html += "  s.style.display = 'block';";
  html += "  setTimeout(() => s.style.display = 'none', 3000);";
  html += "}";
  html += "loadFiles();";
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}

extern "C" {

void httpserver_begin() {
  // Register route handlers - use onNotFound pattern matching
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api", HTTP_GET, handleAPIDocumentation);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/player", HTTP_GET, handlePlayer);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/channels", HTTP_GET, handleChannels);
  server.on("/logs", HTTP_GET, handleLogsPage);
  server.on("/logs/poll", HTTP_GET, handleLogsPoll);
  server.on("/note_on_by_index", HTTP_GET, handleNoteOnByIndex);
  server.on("/note_off_by_index", HTTP_GET, handleNoteOffByIndex);
  server.on("/note_on", HTTP_GET, handleNoteOn);
  server.on("/note_off", HTTP_GET, handleNoteOff);
  server.on("/all_off", HTTP_GET, handleAllOff);
  server.on("/seq_stop", HTTP_GET, handleSeqStop);
  server.on("/seq_pause", HTTP_GET, handleSeqPause);
  server.on("/seq_resume", HTTP_GET, handleSeqResume);
  server.on("/time", HTTP_GET, handleTime);
  server.on("/time/sync", HTTP_GET, handleTimeSync);
  server.on("/time/set", HTTP_POST, handleTimeSet);
  server.on("/time/timezone", HTTP_POST, handleTimeZone);
  server.on("/time/ntp", HTTP_POST, handleTimeNTPServer);
  
  // For parameterized routes, we'll handle them in onNotFound
  // and check the path prefix there
  server.onNotFound([]() {
    handleNotFound();
  });
  
  // Start server
  server.begin();
  // Serial.println("HTTP server started on port 80");
}

void httpserver_loop() {
  server.handleClient();
}

} // extern "C"
