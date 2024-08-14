#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <stddef.h>

#define IPV4_ADDRESS_SIZE 16
#define MAX_METHOD_SIZE 8
#define MAX_PATH_SIZE 1024
#define MAX_VERSION_SIZE 10
#define MAX_HEADER_SIZE 256

typedef struct {
    char method[MAX_METHOD_SIZE];          // HTTP method (e.g., "GET", "POST")
    char path[MAX_PATH_SIZE];              // Request path (e.g., "/index.html")
    char version[MAX_VERSION_SIZE];        // HTTP version (e.g., "HTTP/1.1")
    char host[IPV4_ADDRESS_SIZE];          // Host address or domain (e.g., "example.com")
    char user_agent[MAX_HEADER_SIZE];      // User-Agent header
    char accept[MAX_HEADER_SIZE];          // Accept header
    char content_type[MAX_HEADER_SIZE];    // Content-Type header
    size_t content_length;                 // Length of the message body
    char* body;                            // Message body (e.g., JSON payload for POST requests)
    char* message;                         // Complete HTTP request message
} httprequest_t;

// Function declarations
int httprequest_init(httprequest_t* req);
void httprequest_deinit(httprequest_t* req);
int httprequest_set_method(httprequest_t* req, const char* method);
int httprequest_set_path(httprequest_t* req, const char* path);
int httprequest_set_version(httprequest_t* req, const char* version);
int httprequest_set_host(httprequest_t* req, const char* host);
int httprequest_set_user_agent(httprequest_t* req, const char* user_agent);
int httprequest_set_accept(httprequest_t* req, const char* accept);
int httprequest_set_content_type(httprequest_t* req, const char* content_type);
int httprequest_set_body(httprequest_t* req, const char* body);
int httprequest_build_message(httprequest_t* req);

#endif // HTTPREQUEST_H
