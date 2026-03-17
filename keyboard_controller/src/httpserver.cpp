// httpserver.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "pins.h"
#include "can_bus.h"
#include "key_scanner.h"
#include "api_docs.h"
#include "settings_page.h"
#include "httpserver.h"
#include "logger.h"

static WebServer server(80);

// ---------- Circular log buffer ----------
#define LOG_BUFFER_SIZE 100
static String logBuffer[LOG_BUFFER_SIZE];
static int    logIndex      = 0;
static int    logCount      = 0;
static int    logTotalCount = 0;
static bool   httpLoggingEnabled = true;
static String pendingLogLine     = "";

void httpserver_log(const uint8_t* buffer, size_t size) {
    if (!httpLoggingEnabled) return;
    for (size_t i = 0; i < size; i++) {
        char c = buffer[i];
        if (c == '\n') {
            if (pendingLogLine.length() > 0) {
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

void httpserver_log(const char* message) {
    if (!httpLoggingEnabled) return;
    logBuffer[logIndex] = String(message);
    logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
    if (logCount < LOG_BUFFER_SIZE) logCount++;
    logTotalCount++;
}

static String httpserver_get_logs() {
    String result = "";
    int start = (logCount < LOG_BUFFER_SIZE) ? 0 : logIndex;
    for (int i = 0; i < logCount; i++) {
        int idx = (start + i) % LOG_BUFFER_SIZE;
        result += logBuffer[idx] + "\n";
    }
    return result;
}

static String httpserver_get_logs_since(int sinceTotal) {
    if (sinceTotal < 0) return httpserver_get_logs();
    int newCount = logTotalCount - sinceTotal;
    if (newCount <= 0) return "";
    if (newCount > logCount) return httpserver_get_logs();
    String result = "";
    int start = (logIndex - newCount + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
    for (int i = 0; i < newCount; i++) {
        int idx = (start + i) % LOG_BUFFER_SIZE;
        result += logBuffer[idx] + "\n";
    }
    return result;
}

static int httpserver_get_log_index() {
    return logTotalCount;
}

// ---------- Root – scanner status page ----------
static void handleRoot() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<meta http-equiv='refresh' content='5'>";
    html += "<title>Keyboard Controller</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0;color:#333;}";
    html += "h1{color:#333;}";
    html += ".card{background:white;padding:15px;margin:10px 0;border-radius:8px;"
            "box-shadow:0 2px 4px rgba(0,0,0,.1);}";
    html += ".label{font-weight:bold;margin-right:8px;}";
    html += ".nav{margin-top:15px;}";
    html += ".nav a{display:inline-block;margin:4px;padding:8px 16px;"
            "background:#4CAF50;color:white;text-decoration:none;border-radius:4px;}";
    html += ".nav a:hover{background:#45a049;}";
    html += "</style></head><body>";
    html += "<h1>Keyboard Controller</h1>";
    html += "<div class='card'>";
    html += "<p><span class='label'>MCP23017 chips:</span>" + String(key_scanner_chip_count()) + "</p>";
    html += "<p><span class='label'>Keys scanned:</span>"  + String(key_scanner_chip_count() * 16) + "</p>";
    html += "<p><span class='label'>Hardware ID:</span>"   + String(key_scanner_get_hardware_id()) + "</p>";
    html += "<p><span class='label'>CAN channel:</span>"   + String(key_scanner_get_can_channel()) + "</p>";
    html += "<p><span class='label'>WiFi IP:</span>"       + WiFi.localIP().toString() + "</p>";
    html += "<p><span class='label'>Uptime:</span>"        + String(millis() / 1000) + " s</p>";
    html += "</div>";
    html += "<div class='nav'>";
    html += "<a href='/logs'>Logs</a>";
    html += "<a href='/status'>Status JSON</a>";
    html += "<a href='/api'>API Docs</a>";
    html += "<a href='/settings'>Settings</a>";
    html += "</div>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ---------- Log viewer ----------
static void handleLogsPage() {
    httpLoggingEnabled = true;
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Keyboard Controller Logs</title>";
    html += "<style>";
    html += "body{font-family:monospace;margin:0;padding:20px;background:#1e1e1e;color:#d4d4d4;}";
    html += "h1{color:#4ec9b0;margin-bottom:10px;}";
    html += "#log{background:#252526;border:1px solid #3e3e42;padding:15px;"
            "height:70vh;overflow-y:auto;white-space:pre-wrap;word-wrap:break-word;}";
    html += ".controls{margin-bottom:15px;}";
    html += "button{padding:8px 16px;background:#0e639c;color:white;border:none;"
            "border-radius:4px;cursor:pointer;margin-right:10px;}";
    html += "button:hover{background:#1177bb;}";
    html += ".status{color:#4ec9b0;margin-left:10px;}";
    html += "</style></head><body>";
    html += "<h1>Keyboard Controller Logs</h1>";
    html += "<div class='controls'>";
    html += "<button onclick='clearLog()'>Clear</button>";
    html += "<button onclick='toggleAutoScroll()' id='scrollBtn'>Auto-scroll: ON</button>";
    html += "<a href='/' style='color:#4ec9b0;margin-left:15px;'>&#8592; Home</a>";
    html += "<span class='status' id='status'>Connected</span>";
    html += "</div>";
    html += "<div id='log'></div>";
    html += "<script>";
    html += "let autoScroll=true,lastIndex=-1;";
    html += "const logDiv=document.getElementById('log');";
    html += "const statusDiv=document.getElementById('status');";
    html += "function fetchLogs(){";
    html += "  fetch('/logs/poll?since='+lastIndex).then(r=>r.json()).then(data=>{";
    html += "    if(data.logs){logDiv.textContent+=data.logs;";
    html += "    if(autoScroll)logDiv.scrollTop=logDiv.scrollHeight;}";
    html += "    lastIndex=data.index;";
    html += "    statusDiv.textContent='Connected';statusDiv.style.color='#4ec9b0';";
    html += "  }).catch(()=>{statusDiv.textContent='Error';statusDiv.style.color='#f48771';});";
    html += "}";
    html += "fetchLogs();setInterval(fetchLogs,1000);";
    html += "function clearLog(){logDiv.textContent='';}";
    html += "function toggleAutoScroll(){autoScroll=!autoScroll;";
    html += "  document.getElementById('scrollBtn').textContent='Auto-scroll: '+(autoScroll?'ON':'OFF');}";
    html += "</script></body></html>";
    server.send(200, "text/html", html);
}

static void handleLogsPoll() {
    httpLoggingEnabled = true;
    int sinceIndex = server.hasArg("since") ? server.arg("since").toInt() : -1;
    String logs = httpserver_get_logs_since(sinceIndex);
    int    currentIndex = httpserver_get_log_index();
    if (logs.length() == 0) {
        server.send(200, "application/json",
                    "{\"index\":" + String(currentIndex) + ",\"logs\":\"\"}");
        return;
    }
    String json;
    json.reserve(logs.length() + logs.length() / 5 + 50);
    json = "{\"index\":" + String(currentIndex) + ",\"logs\":\"";
    for (unsigned int i = 0; i < logs.length(); i++) {
        char c = logs[i];
        if      (c == '"')  json += "\\\"";
        else if (c == '\\') json += "\\\\";
        else if (c == '\n') json += "\\n";
        else if (c == '\r') json += "\\r";
        else if (c == '\t') json += "\\t";
        else                json += c;
    }
    json += "\"}";
    server.send(200, "application/json", json);
}

// ---------- Manual note-on / note-off (testing) ----------
static void handleNoteOn() {
    if (!server.hasArg("note")) {
        server.send(400, "text/plain", "Bad Request: Missing 'note' parameter");
        return;
    }
    int note     = server.arg("note").toInt();
    int velocity = server.hasArg("velocity") ? server.arg("velocity").toInt() : 127;
    if (note < 0 || note > 127 || velocity < 0 || velocity > 127) {
        server.send(400, "text/plain", "Bad Request: note 0-127, velocity 0-127");
        return;
    }
    can_send_note_on(key_scanner_get_can_channel(), (uint8_t)note, (uint8_t)velocity);
    server.send(200, "text/plain",
                "Note On: " + String(note) + " vel=" + String(velocity));
}

static void handleNoteOff() {
    if (!server.hasArg("note")) {
        server.send(400, "text/plain", "Bad Request: Missing 'note' parameter");
        return;
    }
    int note     = server.arg("note").toInt();
    int velocity = server.hasArg("velocity") ? server.arg("velocity").toInt() : 64;
    if (note < 0 || note > 127 || velocity < 0 || velocity > 127) {
        server.send(400, "text/plain", "Bad Request: note 0-127, velocity 0-127");
        return;
    }
    can_send_note_off(key_scanner_get_can_channel(), (uint8_t)note, (uint8_t)velocity);
    server.send(200, "text/plain",
                "Note Off: " + String(note) + " vel=" + String(velocity));
}

// ---------- System status ----------
static void handleStatus() {
    String json = "{";
    json += "\"uptime\":"      + String(millis()) + ",";
    json += "\"version\":\""   APP_VERSION "\",";
    json += "\"wifi\":{";
    json += "\"connected\":"   + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
    json += "\"ip\":\""        + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":"        + String(WiFi.RSSI());
    json += "},";
    json += "\"keyScanner\":{";
    json += "\"chips\":"       + String(key_scanner_chip_count()) + ",";
    json += "\"keys\":"        + String(key_scanner_chip_count() * 16) + ",";
    json += "\"hardwareId\":"  + String(key_scanner_get_hardware_id()) + ",";
    json += "\"canChannel\":"  + String(key_scanner_get_can_channel());
    json += "}";
    json += "}";
    server.send(200, "application/json", json);
}

// ---------- Hardware config ----------
static void handleConfig() {
    String json = "{\"hardwareId\":" + String(key_scanner_get_hardware_id()) + "}";
    server.send(200, "application/json", json);
}

static void handleConfigHardwareId() {
    if (!server.hasArg("id")) {
        server.send(400, "text/plain", "Bad Request: Missing 'id' parameter");
        return;
    }
    int id = server.arg("id").toInt();
    if (id < 0 || id > 5) {
        server.send(400, "text/plain", "Bad Request: id must be 0-5");
        return;
    }
    key_scanner_set_hardware_id(id);
    server.send(200, "application/json", "{\"success\":true}");
}

static void handleAPIDocumentation() {
    server.send(200, "text/html", API_DOCS_HTML);
}

static void handleSettings() {
    server.send(200, "text/html", SETTINGS_PAGE_HTML);
}

static void handleNotFound() {
    String message = "Not Found\n\nURI: " + server.uri() + "\n";
    message += "Method: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n";
    server.send(404, "text/plain", message);
}

// ---------- Init / loop ----------
extern "C" {

void httpserver_begin() {
    server.on("/",              HTTP_GET,  handleRoot);
    server.on("/api",           HTTP_GET,  handleAPIDocumentation);
    server.on("/settings",      HTTP_GET,  handleSettings);
    server.on("/status",        HTTP_GET,  handleStatus);
    server.on("/logs",          HTTP_GET,  handleLogsPage);
    server.on("/logs/poll",     HTTP_GET,  handleLogsPoll);
    server.on("/note_on",              HTTP_GET,  handleNoteOn);
    server.on("/note_off",             HTTP_GET,  handleNoteOff);
    server.on("/config",               HTTP_GET,  handleConfig);
    server.on("/config/hardware_id",   HTTP_POST, handleConfigHardwareId);
    server.onNotFound(handleNotFound);
    server.begin();
}

void httpserver_loop() {
    server.handleClient();
}

} // extern "C"
