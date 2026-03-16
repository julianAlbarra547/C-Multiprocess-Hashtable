#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

#define FIFO_CLIENT_PATH "/tmp/client_fifo"
#define FIFO_SERVER_PATH "/tmp/server_fifo"

typedef struct row{
    int id;
    char title[512];
    short rank;
    char date[16];
    char artist[2048];
    char url[128];
    long long streams;
    char album[512];
    double duration;
    char explicito[8];
} Row;

typedef struct {
    char title[512];
    char artist[1024];
} Query;

#endif