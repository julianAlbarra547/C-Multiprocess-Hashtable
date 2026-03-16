#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hash.h"
#include "csv_reader.h"
#include "ipc_protocol.h"

/* normalize_string vive en hash.c pero no está en hash.h */
int normalize_string(char *input, char *output, size_t size);

/* ── Estado global del servidor ─────────────────────────────────────────── */
static long  *g_table;
static FILE  *g_entries;
static FILE  *g_csv;

/* ── I/O robusto ────────────────────────────────────────────────────────── */

static int write_all(int fd, const void *buf, size_t len) {
    const unsigned char *p = buf;
    size_t done = 0;
    while (done < len) {
        ssize_t w = write(fd, p + done, len - done);
        if (w < 0) { if (errno == EINTR) continue; return 0; }
        done += (size_t)w;
    }
    return 1;
}

static int read_all(int fd, void *buf, size_t len) {
    unsigned char *p = buf;
    size_t done = 0;
    while (done < len) {
        ssize_t r = read(fd, p + done, len - done);
        if (r < 0) { if (errno == EINTR) continue; return 0; }
        if (r == 0) return 0;
        done += (size_t)r;
    }
    return 1;
}

/* ── Envío de mensajes ──────────────────────────────────────────────────── */

static int ipc_send(int fd, uint16_t type, const void *payload, uint32_t len) {
    IpcHeader h = { IPC_MAGIC, IPC_VERSION, type, len };
    if (!write_all(fd, &h, sizeof(h))) return 0;
    if (len == 0) return 1;
    return write_all(fd, payload, len);
}

static void send_error(int fd, const char *msg) {
    IpcErrorResp e;
    memset(&e, 0, sizeof(e));
    strncpy(e.message, msg, sizeof(e.message) - 1);
    ipc_send(fd, IPC_MSG_ERROR_RESP, &e, sizeof(e));
}

/* ── Conversión IpcRow ↔ Row ────────────────────────────────────────────── */

static void row_to_ipcrow(const Row *src, IpcRow *dst) {
    memset(dst, 0, sizeof(*dst));
    dst->id       = src->id;
    dst->rank     = src->rank;
    dst->streams  = src->streams;
    dst->duration = src->duration;
    strncpy(dst->title,     src->title,     sizeof(dst->title)     - 1);
    strncpy(dst->date,      src->date,      sizeof(dst->date)      - 1);
    strncpy(dst->artist,    src->artist,    sizeof(dst->artist)    - 1);
    strncpy(dst->url,       src->url,       sizeof(dst->url)       - 1);
    strncpy(dst->album,     src->album,     sizeof(dst->album)     - 1);
    strncpy(dst->explicito, src->explicito, sizeof(dst->explicito) - 1);
}

static void ipcrow_to_row(const IpcRow *src, Row *dst) {
    memset(dst, 0, sizeof(*dst));
    dst->id       = src->id;
    dst->rank     = (short)src->rank;
    dst->streams  = src->streams;
    dst->duration = src->duration;
    strncpy(dst->title,     src->title,     sizeof(dst->title)     - 1);
    strncpy(dst->date,      src->date,      sizeof(dst->date)      - 1);
    strncpy(dst->artist,    src->artist,    sizeof(dst->artist)    - 1);
    strncpy(dst->url,       src->url,       sizeof(dst->url)       - 1);
    strncpy(dst->album,     src->album,     sizeof(dst->album)     - 1);
    strncpy(dst->explicito, src->explicito, sizeof(dst->explicito) - 1);
}

/* ── Búsqueda solo por título (hash.c no tiene esta función) ────────────── */

static int search_by_title(const char *title, long *offsets, int max) {
    char norm[512];
    if (normalize_string((char *)title, norm, sizeof(norm)) != 0) return 0;

    unsigned int index  = hash(norm);
    long         bucket = g_table[index];
    int          found  = 0;

    while (bucket != -1 && found < max) {
        Hash_node node;
        fseek(g_entries, bucket, SEEK_SET);
        fread(&node, sizeof(Hash_node), 1, g_entries);
        if (strcmp(node.title, norm) == 0)
            offsets[found++] = node.offset;
        bucket = node.next;
    }
    return found;
}

/* ── Handlers ───────────────────────────────────────────────────────────── */

static void handle_query(int fd, const IpcQuery *q) {
    IpcQueryResp resp;
    memset(&resp, 0, sizeof(resp));

    if (q->has_artist) {
        /* título + artista: usa search_node de hash.c directamente */
        long offset = search_node(g_table, g_entries,
                                  (char *)q->title, (char *)q->artist);
        if (offset == -1) {
            resp.status = 1;
        } else {
            Row *row = read_csv(g_csv, offset);
            if (!row) { send_error(fd, "Error leyendo CSV."); return; }
            resp.status = 0;
            resp.count  = 1;
            row_to_ipcrow(row, &resp.rows[0]);
            free(row);
        }
    } else {
        /* solo título: recorre el bucket buscando hasta IPC_MAX_RESULTS */
        long offsets[IPC_MAX_RESULTS];
        int  n = search_by_title(q->title, offsets, IPC_MAX_RESULTS);
        if (n == 0) {
            resp.status = 1;
        } else {
            resp.status = 0;
            resp.count  = n;
            for (int i = 0; i < n; i++) {
                Row *row = read_csv(g_csv, offsets[i]);
                if (row) { row_to_ipcrow(row, &resp.rows[i]); free(row); }
            }
        }
    }

    ipc_send(fd, IPC_MSG_QUERY_RESP, &resp, sizeof(resp));
}

static void handle_append(int fd, const IpcRow *ipc_row) {
    Row row;
    ipcrow_to_row(ipc_row, &row);

    char norm_title[512], norm_artist[2048];
    normalize_string(row.title,  norm_title,  sizeof(norm_title));
    normalize_string(row.artist, norm_artist, sizeof(norm_artist));

    if (node_exists(g_table, g_entries, norm_title, norm_artist)) {
        send_error(fd, "La cancion ya existe."); return;
    }

    /* Añadir al CSV */
    if (fseek(g_csv, 0, SEEK_END) != 0) { send_error(fd, "fseek error."); return; }
    long offset = ftell(g_csv);
    int w = fprintf(g_csv, "%d,\"%s\",%d,%s,\"%s\",%s,%lld,\"%s\",%.6f,%s\n",
                    row.id, row.title, row.rank, row.date, row.artist,
                    row.url, row.streams, row.album, row.duration, row.explicito);
    if (w < 0) { send_error(fd, "Error escribiendo CSV."); return; }
    fflush(g_csv);

    /* Insertar en la tabla hash */
    if (insert_node(g_table, g_entries, norm_title, norm_artist, offset) != 0) {
        send_error(fd, "Error insertando en hash."); return;
    }

    /* Persistir índice */
    FILE *idx = fopen(TABLE_IDX, "wb");
    if (idx) { fwrite(g_table, sizeof(long), HASH_TABLE_SIZE, idx); fclose(idx); }

    IpcAppendResp resp = { .status = 0 };
    ipc_send(fd, IPC_MSG_APPEND_RESP, &resp, sizeof(resp));
    printf("[servidor] insertado: '%s' - '%s'\n", row.title, row.artist);
    fflush(stdout);
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(int argc, char *argv[]) {
    if (argc < 2) { fprintf(stderr, "Uso: %s <ruta_csv>\n", argv[0]); return 1; }

    /* Construir índice si no existe */
    FILE *test = fopen(TABLE_IDX, "rb");
    if (!test) {
        printf("[servidor] Construyendo índice...\n"); fflush(stdout);
        if (build_index(argv[1], TABLE_IDX, ENTRIES_BIN) != 0) {
            fprintf(stderr, "[servidor] Error construyendo índice.\n"); return 1;
        }
    } else { fclose(test); }

    /* Cargar tabla */
    g_table = malloc(HASH_TABLE_SIZE * sizeof(long));
    if (!g_table || load_table(TABLE_IDX, g_table) != 0) {
        fprintf(stderr, "[servidor] Error cargando tabla.\n"); return 1;
    }

    g_entries = fopen(ENTRIES_BIN, "r+b");
    g_csv     = fopen(argv[1], "a+");
    if (!g_entries || !g_csv) { perror("fopen"); return 1; }

    /* Crear FIFOs */
    if (mkfifo(FIFO_UI_TO_HASH, 0666) != 0 && errno != EEXIST) { perror("mkfifo"); return 1; }
    if (mkfifo(FIFO_HASH_TO_UI, 0666) != 0 && errno != EEXIST) { perror("mkfifo"); return 1; }

    printf("[servidor] Listo en %s\n", FIFO_UI_TO_HASH); fflush(stdout);

    /* Bucle principal */
    while (1) {
        int fd_req = open(FIFO_UI_TO_HASH, O_RDONLY);
        if (fd_req < 0) { perror("open req"); break; }

        int fd_resp = open(FIFO_HASH_TO_UI, O_WRONLY);
        if (fd_resp < 0) { perror("open resp"); close(fd_req); break; }

        IpcHeader h;
        if (!read_all(fd_req, &h, sizeof(h)) ||
            h.magic != IPC_MAGIC || h.version != IPC_VERSION) {
            close(fd_req); close(fd_resp); continue;
        }

        /* Buffer de payload — el más grande posible entre Query e IpcRow */
        unsigned char payload[sizeof(IpcQuery) > sizeof(IpcRow)
                               ? sizeof(IpcQuery) : sizeof(IpcRow)];

        if (h.payload_len > sizeof(payload) ||
            (h.payload_len > 0 && !read_all(fd_req, payload, h.payload_len))) {
            close(fd_req); close(fd_resp); continue;
        }
        close(fd_req);

        switch (h.type) {
            case IPC_MSG_QUERY:
                handle_query(fd_resp, (const IpcQuery *)payload);  break;
            case IPC_MSG_APPEND:
                handle_append(fd_resp, (const IpcRow *)payload);   break;
            default:
                send_error(fd_resp, "Tipo de mensaje desconocido.");
        }

        close(fd_resp);
    }

    fclose(g_entries);
    fclose(g_csv);
    free(g_table);
    unlink(FIFO_UI_TO_HASH);
    unlink(FIFO_HASH_TO_UI);
    return 0;
}