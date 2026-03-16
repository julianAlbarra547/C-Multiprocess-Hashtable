#ifndef IPC_PROTOCOL_H
#define IPC_PROTOCOL_H

#include <stdint.h>

#define MAX_TITLE 512
#define MAX_ARTIST 2048
#define MAX_RESULTS 5

typedef enum
{
    OP_QUERY = 1,
    OP_APPEND = 2
} OpType;

typedef struct
{
    int id;
    char title[MAX_TITLE];
    int16_t rank;
    char date[16];
    char artist[MAX_ARTIST];
    char url[128];
    int64_t streams;
    char album[512];
    double duration;
    char explicito[8];
} IpcRow;

typedef struct
{
    OpType type;

    char title[MAX_TITLE];
    char artist[MAX_ARTIST];

    int has_artist;

} IpcQuery;

typedef struct
{
    int status;
    int count;

    IpcRow rows[MAX_RESULTS];

} IpcQueryResp;

typedef struct
{
    int status;

} IpcAppendResp;

#endif