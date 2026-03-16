#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "hash.h"
#include "csv_reader.h"
#include "ipc_protocol.h"

#define FIFO_REQ "/tmp/music_req"
#define FIFO_RESP "/tmp/music_resp"
#define CSV_FILE "spotify.csv"

typedef struct
{
    OpType type;

    union
    {
        IpcQuery query;
        IpcRow row;
    };

} Request;

int main()
{
    mkfifo(FIFO_REQ, 0666);
    mkfifo(FIFO_RESP, 0666);

    int req_fd = open(FIFO_REQ, O_RDWR);
    int resp_fd = open(FIFO_RESP, O_RDWR);

    long table[HASH_TABLE_SIZE];

    if (load_table(TABLE_IDX, table) != 0)
    {
        fprintf(stderr, "Error cargando índice\n");
        return 1;
    }

    FILE *entries = fopen(ENTRIES_BIN, "rb+");
    FILE *csv = open_csv(CSV_FILE);

    if (!entries || !csv)
    {
        fprintf(stderr, "Error abriendo archivos\n");
        return 1;
    }

    while (1)
    {
        Request req;

        if (read(req_fd, &req, sizeof(req)) <= 0)
            continue;

        if (req.type == OP_QUERY)
        {
            IpcQueryResp resp;

            memset(&resp, 0, sizeof(resp));

            if (req.query.has_artist)
            {
                long offset = search_node(
                    table,
                    entries,
                    req.query.title,
                    req.query.artist);

                if (offset == -1)
                {
                    resp.status = 1;
                }
                else
                {
                    Row *row = read_csv(csv, offset);

                    resp.status = 0;
                    resp.count = 1;

                    resp.rows[0].id = row->id;
                    strcpy(resp.rows[0].title, row->title);
                    resp.rows[0].rank = row->rank;
                    strcpy(resp.rows[0].date, row->date);
                    strcpy(resp.rows[0].artist, row->artist);
                    strcpy(resp.rows[0].url, row->url);
                    resp.rows[0].streams = row->streams;
                    strcpy(resp.rows[0].album, row->album);
                    resp.rows[0].duration = row->duration;
                    strcpy(resp.rows[0].explicito, row->explicito);

                    free(row);
                }
            }
            else
            {
                Hash_node nodes[5];

                int count = search_range_node(
                    table,
                    entries,
                    req.query.title,
                    nodes,
                    5);

                resp.status = 0;
                resp.count = count;

                for (int i = 0; i < count; i++)
                {
                    Row *row = read_csv(csv, nodes[i].offset);

                    resp.rows[i].id = row->id;
                    strcpy(resp.rows[i].title, row->title);
                    resp.rows[i].rank = row->rank;
                    strcpy(resp.rows[i].date, row->date);
                    strcpy(resp.rows[i].artist, row->artist);
                    strcpy(resp.rows[i].url, row->url);
                    resp.rows[i].streams = row->streams;
                    strcpy(resp.rows[i].album, row->album);
                    resp.rows[i].duration = row->duration;
                    strcpy(resp.rows[i].explicito, row->explicito);

                    free(row);
                }
            }

            write(resp_fd, &resp, sizeof(resp));
        }

        if (req.type == OP_APPEND)
        {
            IpcAppendResp resp;

            fseek(csv, 0, SEEK_END);

            long offset = ftell(csv);

            IpcRow *r = &req.row;

            fprintf(csv,
                    "%d,%s,%d,%s,%s,%s,%lld,%s,%f,%s\n",
                    r->id,
                    r->title,
                    r->rank,
                    r->date,
                    r->artist,
                    r->url,
                    r->streams,
                    r->album,
                    r->duration,
                    r->explicito);

            fflush(csv);

            insert_node(
                table,
                entries,
                r->title,
                r->artist,
                offset);

            resp.status = 0;

            write(resp_fd, &resp, sizeof(resp));
        }
    }

    return 0;
}