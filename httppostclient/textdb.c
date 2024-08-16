#include "textdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* data = (char*)malloc(size + 1);
    if (data == NULL) {
        perror("textdb_init: Failed to allocate memory");
        fclose(file);
        return -1;
    }

    fread(data, 1, size, file);
    data[size] = '\0';

    fclose(file);

    textdb->data = data;
    textdb->size = size;

    printf("textdb_init: Successfully initialized textdb with size %zu\n", size);
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
        printf("textdb_deinit: Successfully deinitialized textdb\n");
    } else {
        printf("textdb_deinit: No data to free\n");
    }

    return 0;
}

int textdb_gethost(textdb_t* textdb, hostinfor_t* host, int index) {
    char buff[25];
    if (textdb == NULL || host == NULL || index < 0) {
        fprintf(stderr, "textdb_gethost: Invalid arguments\n");
        return -1;
    }

    size_t addr = index * (TXT_HOST_INFO_MAX_LENGH);
    if (addr > textdb->size - sizeof(hostinfor_t)) {
        return -1;
    }

    memcpy(buff, &textdb->data[addr], TXT_HOST_INFO_MAX_LENGH);
    const char* colon = strchr(buff, ':');
    size_t ip_len;
    if (colon == NULL) {
        const char* space = strchr(buff, ' ');
        ip_len = space - buff;
        host->port = 80;
    } else {
        ip_len = colon - buff;
        host->port = atoi(colon + 1);
    }

    if (ip_len >= IPV4_ADRESS_SIZE) {
        return -1;
    }

    strncpy(host->ip, buff, ip_len);
    host->ip[ip_len] = '\0';

    return 0;
}
