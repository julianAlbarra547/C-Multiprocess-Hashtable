#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "ipc_client.h"
#include "ipc_protocol.h"

#define FIFO_REQ "/tmp/music_req"
#define FIFO_RESP "/tmp/music_resp"

struct IpcClient
{
    int req_fd;
    int resp_fd;
};

IpcClient *ipc_client_connect(const char *endpoint,
                              char *err_buf,
                              size_t err_buf_size)
{
    (void)endpoint;

    IpcClient *client = malloc(sizeof(IpcClient));

    if (!client)
    {
        fprintf(stderr, "Error reservando memoria\n");
        return NULL;
    }

    client->req_fd = open(FIFO_REQ, O_WRONLY);
    client->resp_fd = open(FIFO_RESP, O_RDONLY);

    if (client->req_fd < 0 || client->resp_fd < 0)
    {
        fprintf(stderr, "Error conectando a FIFO\n");
        free(client);
        return NULL;
    }

    return client;
}

void ipc_client_close(IpcClient *client)
{
    if (!client)
        return;

    close(client->req_fd);
    close(client->resp_fd);

    free(client);
}

int ipc_client_query(IpcClient *client,
                     const IpcQuery *query,
                     IpcQueryResp *resp,
                     char *err_buf,
                     size_t err_buf_size)
{
    if (write(client->req_fd, query, sizeof(*query)) < 0)
    {
        fprintf(stderr, "Error enviando query\n");
        return 0;
    }

    if (read(client->resp_fd, resp, sizeof(*resp)) <= 0)
    {
        fprintf(stderr, "Error leyendo respuesta\n");
        return 0;
    }

    return 1;
}

int ipc_client_append(IpcClient *client,
                      const IpcRow *row,
                      IpcAppendResp *resp,
                      char *err_buf,
                      size_t err_buf_size)
{
    struct
    {
        OpType type;
        IpcRow row;
    } msg;

    msg.type = OP_APPEND;
    msg.row = *row;

    if (write(client->req_fd, &msg, sizeof(msg)) < 0)
    {
        fprintf(stderr, "Error enviando append\n");
        return 0;
    }

    if (read(client->resp_fd, resp, sizeof(*resp)) <= 0)
    {
        fprintf(stderr, "Error leyendo respuesta\n");
        return 0;
    }

    return 1;
}