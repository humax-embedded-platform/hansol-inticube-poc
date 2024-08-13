#include "httprequest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_METHOD "POST"
#define DEFAULT_VERSION "HTTP/1.1"
#define DEFAULT_USER_AGENT "marusys"
#define DEFAULT_ACCEPT "*/*"
#define DEFAULT_CONTENT_TYPE "application/x-www-form-urlencoded"

int httprequest_init(httprequest_t* req) {
    if (req == NULL) {
        return -1;
    }

    strncpy(req->method, DEFAULT_METHOD, sizeof(req->method));
    strncpy(req->version, DEFAULT_VERSION, sizeof(req->version));
    strncpy(req->user_agent, DEFAULT_USER_AGENT, sizeof(req->user_agent));
    strncpy(req->accept, DEFAULT_ACCEPT, sizeof(req->accept));
    strncpy(req->content_type, DEFAULT_CONTENT_TYPE, sizeof(req->content_type));
    req->content_length = 0;
    req->body = NULL;
    req->message = NULL;

    return 0;
}

void httprequest_deinit(httprequest_t* req) {
    if (req == NULL) {
        return;
    }

    if (req->body) {
        free(req->body);
        req->body = NULL;
    }
    if (req->message) {
        free(req->message);
        req->message = NULL;
    }
}

// Setters for various fields
int httprequest_set_method(httprequest_t* req, const char* method) {
    if (req == NULL || method == NULL) {
        return -1;
    }
    strncpy(req->method, method, sizeof(req->method));
    return 0;
}

int httprequest_set_path(httprequest_t* req, const char* path) {
    if (req == NULL || path == NULL) {
        return -1;
    }
    strncpy(req->path, path, sizeof(req->path));
    return 0;
}

int httprequest_set_version(httprequest_t* req, const char* version) {
    if (req == NULL || version == NULL) {
        return -1;
    }
    strncpy(req->version, version, sizeof(req->version));
    return 0;
}

int httprequest_set_host(httprequest_t* req, const char* host) {
    if (req == NULL || host == NULL) {
        return -1;
    }
    strncpy(req->host, host, sizeof(req->host));
    return 0;
}

int httprequest_set_user_agent(httprequest_t* req, const char* user_agent) {
    if (req == NULL || user_agent == NULL) {
        return -1;
    }
    strncpy(req->user_agent, user_agent, sizeof(req->user_agent));
    return 0;
}

int httprequest_set_accept(httprequest_t* req, const char* accept) {
    if (req == NULL || accept == NULL) {
        return -1;
    }
    strncpy(req->accept, accept, sizeof(req->accept));
    return 0;
}

int httprequest_set_content_type(httprequest_t* req, const char* content_type) {
    if (req == NULL || content_type == NULL) {
        return -1;
    }
    strncpy(req->content_type, content_type, sizeof(req->content_type));
    return 0;
}

int httprequest_set_body(httprequest_t* req, const char* body) {
    if (req == NULL || body == NULL) {
        return -1;
    }

    if (req->body) {
        free(req->body);
    }

    req->content_length = strlen(body);
    req->body = malloc(req->content_length + 1);  // +1 for null terminator
    if (req->body == NULL) {
        return -1;
    }

    strcpy(req->body, body);
    return 0;
}

int httprequest_build_message(httprequest_t* req) {
    if (req == NULL) {
        return -1;
    }

    if (req->message) {
        free(req->message);
    }

    size_t message_size = snprintf(NULL, 0,
        "%s %s %s\r\n"
        "Host: %s\r\n"
        "User-Agent: %s\r\n"
        "Accept: %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n\r\n%s",
        req->method, req->path, req->version,
        req->host,
        req->user_agent,
        req->accept,
        req->content_type,
        req->content_length,
        req->body ? req->body : "");

    req->message = malloc(message_size + 1);
    if (req->message == NULL) {
        return -1;
    }

    // Build the message
    snprintf(req->message, message_size + 1,
        "%s %s %s\r\n"
        "Host: %s\r\n"
        "User-Agent: %s\r\n"
        "Accept: %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n\r\n%s",
        req->method, req->path, req->version,
        req->host,
        req->user_agent,
        req->accept,
        req->content_type,
        req->content_length,
        req->body ? req->body : "");

    return 0;
}
