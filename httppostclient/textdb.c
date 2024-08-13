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
    if (textdb == NULL || host == NULL || index < 0) {
        fprintf(stderr, "textdb_gethost: Invalid arguments\n");
        return -1;
    }

    size_t addr = index * (sizeof(hostinfor_t));
    if (addr > textdb->size - sizeof(hostinfor_t)) {
        fprintf(stderr, "textdb_gethost: Index out of range (index=%d, addr=%zu, size=%zu)\n", index, addr, textdb->size);
        return -1;
    }

    memcpy(host->ip, &textdb->data[addr], sizeof(hostinfor_t));

    for (size_t i = sizeof(hostinfor_t) -1; i >=0; i--) {
        if (host->ip[i] == ' ') {
            host->ip[i] = '\0';
        }
    }

    return 0;
}
