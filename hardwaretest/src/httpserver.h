#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the HTTP server on port 80
void httpserver_begin(void);

// Handle HTTP client requests (call frequently in main loop)
void httpserver_loop(void);

#ifdef __cplusplus
}

// C++ only functions (need C++ linkage for overloading)
void httpserver_log(const uint8_t* buffer, size_t size);
void httpserver_log(const char* message);
#endif

#endif // HTTPSERVER_H
