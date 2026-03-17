#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common.h"
#include "csv_reader.h"
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "hash.h"

#define CSV_FILE "data/sample/spotify_data_sample.csv"

int main(){
    Query query;
    int identify;
    int r, fdread, fdwrite;

    r = build_index(CSV_FILE, TABLE_IDX, ENTRIES_BIN);
    if (r == -1) {
        fprintf(stderr, "Error building index\n");
        return 1;
    }

    r = mkfifo(FIFO_CLIENT_PATH, 0666);
    if (r == -1) {
        perror("mkfifo read");
        return -1;
    }
    
    r = mkfifo(FIFO_SERVER_PATH, 0666);
    if (r == -1) {
        perror("mkfifo write");
        return -1;
    }

    fdread = open(FIFO_CLIENT_PATH, O_RDONLY);
    if (fdread == -1) {
        perror("Open read fifo");
        return -1;
    }

    fdwrite = open(FIFO_SERVER_PATH, O_WRONLY);
    if (fdwrite == -1) {
        perror("Open write fifo");
        return -1;
    }

    long table[HASH_TABLE_SIZE];
    if (load_table(TABLE_IDX, table) != 0) {
        fprintf(stderr, "Error cargando índice\n");
        return 1;
    }

    FILE *entries_file = fopen(ENTRIES_BIN, "rb+");;
    if (!entries_file) {
        fprintf(stderr, "Error abriendo archivo de entradas\n");
        return 1;
    }

    while(1){

        if (read(fdread, &identify, sizeof(int)) <= 0) {
            perror("Error reading identity from fifo");
            continue;
        }

        if (identify == 1){
            if (read(fdread, &query, sizeof(Query)) <= 0) {
                perror("Error reading query from fifo");
                continue;
            }

            printf("Received query: Title='%s', Artist='%s'\n", query.title, query.artist);

            if (strlen(query.title) == 0 || strlen(query.artist) == 0) {
                fprintf(stderr, "Invalid query: Title and Artist cannot be empty.\n");
                continue;
            }

            if(query.artist == NULL || query.artist[0] == '\0'){
                //Buscar solo por titulo
                Hash_node nodes[5];
                int count = search_range_node(table, entries_file, query.title, nodes, 5);

                if (count == -1) {
                    fprintf(stderr, "Error searching for title: %s\n", query.title);
                    continue;
                } else if (count <= 5){
                    printf("It was found %d entries for title: %s\n", count, query.title);
                }

                if (write(fdwrite, &nodes, sizeof(Hash_node) * count) == -1) {
                    perror("Error writing to fifo");
                    continue;
                }

            } else {
                //Buscar por titulo y artista
                long offset = search_node(table, entries_file, query.title, query.artist);
                if (offset == -1) {
                    fprintf(stderr, "No entry found for Title='%s' and Artist='%s'\n", query.title, query.artist);
                    continue;
                }
                Row *row;
                row = read_csv(entries_file, offset);

                if (write(fdwrite, row, sizeof(Row)) == -1) {
                    perror("Error writing row to fifo");
                    free(row);
                    continue;
                }
                free(row);
            }

        } else if (identify == 2){
            // Agregar cancion
            Row new_row;
            if (read(fdread, &new_row, sizeof(Row)) <= 0) {
                perror("Error reading new row from fifo");
                continue;
            }

            if (search_node(table, entries_file, new_row.title, new_row.artist) != -1) {
                fprintf(stderr, "Error: A song with Title='%s' and Artist='%s' already exists.\n", new_row.title, new_row.artist);
                continue;
            }

            FILE *csv = fopen(CSV_FILE, "a");
            if (!csv) {
                fprintf(stderr, "Error abriendo archivo CSV para agregar nueva canción\n");
                continue;
            }

            fprintf(csv,
                    "%d,%s,%d,%s,%s,%s,%ld,%s,%f,%s\n",
                    new_row.id,
                    new_row.title,
                    new_row.rank,
                    new_row.date,
                    new_row.artist,
                    new_row.url,
                    new_row.streams,
                    new_row.album,
                    new_row.duration,
                    new_row.explicito);
            fflush(csv);
            fclose(csv);
                // Actualizar índice
            char norm_title[512], norm_artist[2048];
            normalize_string(new_row.title, norm_title, sizeof(norm_title));
            normalize_string(new_row.artist, norm_artist, sizeof(norm_artist));
            insert_node(table, entries_file, norm_title, norm_artist, ftell(csv));


            // Guardar tabla actualizada
            FILE *idx = fopen(TABLE_IDX, "wb");
            if (!idx) {
                fprintf(stderr, "Error abriendo archivo de índice para actualización\n");
                continue;
            }
            fwrite(table, sizeof(long), HASH_TABLE_SIZE, idx);

            int confirm = 1;
            write(fdwrite, &confirm, sizeof(int));
            fclose(idx);

        } else {
            fprintf(stderr, "Invalid identify value: %d\n", identify);
        }

    }

    return 0;
}