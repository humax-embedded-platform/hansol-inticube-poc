#ifndef MESSAGE_H
#define MESSAGE_H

typedef struct usermsg {
    char* msg;
} usermsg_t;

int message_init(usermsg_t* msg, const char* filepath);
void message_deinit(usermsg_t* msg);


#endif