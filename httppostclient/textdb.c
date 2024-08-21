#include "textdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_LINE_LENGTH 256


int is_ipv4_address(const char* str) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, str, &(sa.sin_addr));
}

int is_http_address(const char* str) {
    char* prefix = "http://";
    if(strncmp(str, prefix, strlen(prefix)) == 0) {
        return 1;
    }

    return 0;
}

int is_https_address(const char* str) {
    char* prefix = "https://";
    if(strncmp(str, prefix, strlen(prefix)) == 0) {
        return 1;
    }

    return 0;
}

uint16_t get_port_info(const char* str) {
    uint16_t port = 0;
    char* colon = strchr(str, ':');
    if (colon) {
        port = (uint16_t)atoi(colon + 1);  // Extract port number
    }

    return port;
}

uint16_t get_host_length(const char* str) {
    char* colon = strchr(str, ':');
    if (colon) {
        return colon - str;
    }
    return strlen(str);
}


int textdb_init(textdb_t* textdb, const char* dbpath) {
    if (textdb == NULL || dbpath == NULL) {
        fprintf(stderr, "textdb_init: Invalid arguments\n");
        return -1;
    }

    FILE* file = fopen(dbpath, "r");
    if (file == NULL) {
        perror("textdb_init: Failed to open file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* data = (char*)malloc(file_size*2);
    if (data == NULL) {
        perror("textdb_init: Failed to allocate memory");
        fclose(file);
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    size_t offset = 0;
    uint16_t port = 0;
    char address_type = HOST_IPV4_TYPE;
    char protocol = HTTP_PROTOCOL;
    while (fgets(line, sizeof(line), file)) {
        size_t line_len = strlen(line);
        if (line[line_len - 1] == '\n') {
            line_len--;
            line[line_len] = '\0';
        }

        port = get_port_info(line);

        if (is_ipv4_address(line)) {
            if(port == 0) {
                printf("host ip should include port number: %s\n", line);
                continue;
            }

            address_type = HOST_IPV4_TYPE;
            protocol = HTTP_PROTOCOL;
        } else if(is_http_address(line)) {
            if(port == 0) {
                port = HTTP_DEFAILT_PORT;
                address_type = HOST_DOMAIN_TYPE;
                protocol = HTTP_PROTOCOL;
            }
        } else if(is_https_address(line)){
            if(port == 0) {
                port = HTTPS_DEFAULT_PORT;
                address_type = HOST_DOMAIN_TYPE;
                protocol = HTTPS_PROTOCOL;
            }
        } else {
            if(port == 0) { 
                port = HTTP_DEFAILT_PORT;
                address_type = HOST_DOMAIN_TYPE;
                protocol = HTTP_PROTOCOL;
            }
        }

        int host_len = get_host_length(line);

        uint16_t total_len = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(char) + sizeof(char) + host_len;

        memcpy((char*)data + offset, &total_len, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        memcpy((char*)data + offset, &port, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        memcpy((char*)data + offset, &address_type, sizeof(char));
        offset += sizeof(char);

        memcpy((char*)data + offset, &protocol, sizeof(char));
        offset += sizeof(char);

        memcpy((char*)data + offset, line, host_len);
        offset += host_len;
    }

    data = realloc(data, offset);
    if (!data) {
        perror("Failed to reallocate memory");
        free(data);
        fclose(file);
        return -1;
    }

    textdb->data = data;
    textdb->size = offset;
    textdb->read_offset = 0;

    if (pthread_mutex_init(&textdb->read_lock, NULL) != 0) {
        perror("textdb_init: Mutex initialization failed");
        free(data);
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

int textdb_deinit(textdb_t* textdb) {
    if (textdb == NULL) {
        fprintf(stderr, "textdb_deinit: Invalid argument\n");
        return -1;
    }

    if (textdb->data != NULL) {
        free(textdb->data);
        textdb->data = NULL;
        textdb->size = 0;
        printf("textdb_deinit: Done\n");
    } else {
        printf("textdb_deinit: No data to free\n");
    }

    pthread_mutex_destroy(&textdb->read_lock);

    return 0;
}

int textdb_gethost(textdb_t* textdb, hostinfor_t* host) {
    char buff[25];
    uint16_t data_len;
    uint16_t current_read_offset = 0;

    if (textdb == NULL || host == NULL) {
        fprintf(stderr, "textdb_gethost: Invalid arguments\n");
        return -1;
    }

    if (pthread_mutex_lock(&textdb->read_lock) != 0) {
        perror("textdb_gethost: Mutex lock failed");
        return -1;
    }

    if(textdb->read_offset >= (textdb->size -1)) {
        textdb->read_offset = 0;
    }

    current_read_offset = textdb->read_offset;
    data_len = *(uint16_t*)(textdb->data + textdb->read_offset);
    textdb->read_offset = current_read_offset + data_len;

    pthread_mutex_unlock(&textdb->read_lock);

    host->port = *(uint16_t*)(textdb->data + current_read_offset +2); // port
    host->adress_type = *(char*)(textdb->data + current_read_offset +4); // adress type
    host->protocol = *(char*)(textdb->data + current_read_offset +5); // protocol
    if(host->adress_type == HOST_IPV4_TYPE) {
        memcpy(host->adress.ip, textdb->data + current_read_offset + 6, data_len-6);
        host->adress.ip[data_len-6] = '\0';
    } else {
        memcpy(host->adress.domain, textdb->data + current_read_offset + 6, data_len-6);
        host->adress.domain[data_len-6] = '\0';
    }

    return 0;
}
