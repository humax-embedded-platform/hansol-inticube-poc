#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void message_init(usermsg_t* msg, const char* filepath) {
    if (msg == NULL || filepath == NULL) {
        return;
    }

    FILE* file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Failed to open file");
        msg->msg = NULL;
        return;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (filesize < 0) {
        perror("Failed to determine file size");
        fclose(file);
        msg->msg = NULL;
        return;
    }

    msg->msg = (char*)malloc(filesize + 1);
    if (msg->msg == NULL) {
        perror("Failed to allocate memory");
        fclose(file);
        return;
    }

    size_t read_size = fread(msg->msg, 1, filesize, file);
    if (read_size != (size_t)filesize) {
        perror("Failed to read file content");
        free(msg->msg);
        msg->msg = NULL;
        fclose(file);
        return;
    }

    msg->msg[filesize] = '\0';

    fclose(file);
}

void message_deinit(usermsg_t* msg) {
    if (msg == NULL) {
        return;
    }

    if (msg->msg != NULL) {
        free(msg->msg);
        msg->msg = NULL;
    }
}
