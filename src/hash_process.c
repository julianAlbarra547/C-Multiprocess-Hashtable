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


int main(){
    Query query;
    int identify;
    int r, fdread, fdwrite;

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

    long entries_file = open(ENTRIES_BIN, O_RDWR);
    if (entries_file == -1) {
        fprintf(stderr, "Error abriendo archivo de entradas\n");
        return 1;
    }

    while(1){

        if (read(fdread, &identify, sizeof(int) <=0)){
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
                    fprintf("It was found %d entries for title: %s\n", count, query.title);
                }

                if (write(fdwrite, &nodes, sizeof(Hash_node) * count) == -1) {
                    perror("Error writing to fifo");
                    continue;
                }

            } else {
                //Buscar por titulo y artista
                
            }

        } else if (identify == 2){
            // Agregar cancion
        } else {
            fprintf(stderr, "Invalid identify value: %d\n", identify);
        }

    }


    return 0;
}