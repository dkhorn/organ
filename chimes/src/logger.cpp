// logger.cpp - Unified logging implementation
#include "logger.h"
#include "httpserver.h"

UnifiedLogger Log;

size_t UnifiedLogger::write(uint8_t c) {
  size_t n = Serial.write(c);
  if (telnetClient && telnetClient->connected()) {
    telnetClient->write(c);
  }
  return n;
}

size_t UnifiedLogger::write(const uint8_t *buffer, size_t size) {
  size_t n = Serial.write(buffer, size);
  if (telnetClient && telnetClient->connected()) {
    telnetClient->write(buffer, size);
  }
  
  // Send raw buffer to HTTP logger (it will handle String building if enabled)
  httpserver_log(buffer, size);
  
  return n;
}

void UnifiedLogger::setTelnetClient(WiFiClient* client) {
  telnetClient = client;
}
