#ifndef IPC_CLIENT_H
#define IPC_CLIENT_H

#include <stddef.h>

#include "../src/ipc_protocol.h"

typedef struct IpcClient IpcClient;

/* Connect to an IPC service/daemon (implementation-defined).
   - endpoint: optional string (NULL/empty = default).
   - err_buf: optional; receives a human-readable error message on failure.
   Returns: non-NULL on success. */
IpcClient *ipc_client_connect(const char *endpoint, char *err_buf, size_t err_buf_size);

/* Close and free client resources. Safe to call with NULL. */
void ipc_client_close(IpcClient *client);

/* Query the server. Returns 1 on success (resp filled), 0 on failure (err_buf filled). */
int ipc_client_query(IpcClient *client, const IpcQuery *query, IpcQueryResp *resp, char *err_buf, size_t err_buf_size);

/* Append a row. Returns 1 on success (resp filled), 0 on failure (err_buf filled). */
int ipc_client_append(IpcClient *client, const IpcRow *row, IpcAppendResp *resp, char *err_buf, size_t err_buf_size);

#endif

