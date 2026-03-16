#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

#define FIFO_CLIENT_PATH "/tmp/client_fifo"
#define FIFO_SERVER_PATH "/tmp/server_fifo"
typedef struct {
    char title[512];
    char artist[1024];
} Query;

#endif