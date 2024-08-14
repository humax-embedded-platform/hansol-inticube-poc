#ifndef TEXTDB_H
#define TEXTDB_H

#include <stddef.h>
#include "common.h"

#define TXT_HOST_INFO_MAX_LENGH  25

int textdb_init(textdb_t* db, const char* dbpath);

int textdb_deinit(textdb_t* textdb);

int textdb_gethost(textdb_t* textdb, hostinfor_t* host, int index);

#endif // TEXTDB_H
