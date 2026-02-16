// logger.h - Unified logging to Serial, Telnet, and HTTP
#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <WiFi.h>

#ifdef __cplusplus
extern "C" {
#endif

// Custom print class that outputs to Serial, Telnet, and HTTP log buffer
class UnifiedLogger : public Print {
public:
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  
  void setTelnetClient(WiFiClient* client);
  
private:
  WiFiClient* telnetClient = nullptr;
};

// Global logger instance
extern UnifiedLogger Log;

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H
