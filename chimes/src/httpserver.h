#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the HTTP server on port 80
void httpserver_begin(void);

// Handle HTTP client requests (call frequently in main loop)
void httpserver_loop(void);

#ifdef __cplusplus
}
#endif

#endif // HTTPSERVER_H
