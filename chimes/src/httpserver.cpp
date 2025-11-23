// httpserver.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "chimes.h"
#include "midinote.h"
#include "keyboard.h"
#include "midiseq.h"
#include "songs.h"

static WebServer server(80);

// Handler for GET /
static void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += "h1 { color: #333; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }";
  html += ".button-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(80px, 1fr)); gap: 10px; margin-top: 20px; }";
  html += "button { padding: 15px; font-size: 16px; border: 2px solid #4CAF50; background: #4CAF50; color: white; border-radius: 5px; cursor: pointer; transition: all 0.3s; }";
  html += "button:hover { background: #45a049; transform: scale(1.05); }";
  html += "button:active { transform: scale(0.95); }";
  html += ".status { margin-top: 15px; padding: 10px; background: #e8f5e9; border-radius: 5px; display: none; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Chime Controller</h1>";
  html += "<p>Click a button to ring a chime by channel (0-20)</p>";
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
  html += "  fetch('/ringchannel/' + note)";
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
  
  ring_chime(note);
  
  String response = "Rang note " + String(note);
  server.send(200, "text/plain", response);
}

// Handler for GET /ringchannel/<channel>
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
  
  // Ring the physical channel directly (bypass note mapping)
  ring_chime_by_channel(channel);
  
  String response = "Rang physical channel " + String(channel);
  server.send(200, "text/plain", response);
}

// Handler for GET /keyboard
static void handleKeyboard() {
  server.send(200, "text/html", get_keyboard_html());
}

// Handler for GET /note_on?note=X&velocity=Y
static void handleNoteOn() {
  if (!server.hasArg("note")) {
    server.send(400, "text/plain", "Bad Request: Missing 'note' parameter");
    return;
  }
  
  int note = server.arg("note").toInt();
  int velocity = server.hasArg("velocity") ? server.arg("velocity").toInt() : 100;
  
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
  Serial.println("All notes off (panic)");
  server.send(200, "text/plain", "All notes off");
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
  Serial.println(response);
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
  Serial.println("Sequence stopped");
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

extern "C" {

void httpserver_begin() {
  // Register route handlers - use onNotFound pattern matching
  server.on("/", HTTP_GET, handleRoot);
  server.on("/keyboard", HTTP_GET, handleKeyboard);
  server.on("/note_on", HTTP_GET, handleNoteOn);
  server.on("/note_off", HTTP_GET, handleNoteOff);
  server.on("/all_off", HTTP_GET, handleAllOff);
  server.on("/songs", HTTP_GET, handleListSongs);
  server.on("/seq_stop", HTTP_GET, handleSeqStop);
  server.on("/seq_pause", HTTP_GET, handleSeqPause);
  server.on("/seq_resume", HTTP_GET, handleSeqResume);
  
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
  Serial.println("HTTP server started on port 80");
}

void httpserver_loop() {
  server.handleClient();
}

} // extern "C"
