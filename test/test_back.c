#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "../src/common.h"
#include "../src/csv_reader.h"

#define FIFO_READ_PATH  FIFO_CLIENT_PATH
#define FIFO_WRITE_PATH FIFO_SERVER_PATH

int main(){
    int fdread, fdwrite, r;

    /* Crear FIFOs si no existen */
    r = mkfifo(FIFO_READ_PATH, 0666);
    if(r == -1 && errno != EEXIST){
        perror("mkfifo read");
        return -1;
    }

    r = mkfifo(FIFO_WRITE_PATH, 0666);
    if(r == -1 && errno != EEXIST){
        perror("mkfifo write");
        return -1;
    }

    printf("Servidor de prueba iniciado. Esperando conexion de la UI...\n");

    fdread = open(FIFO_READ_PATH, O_RDONLY);
    if(fdread == -1){
        perror("open read fifo");
        return -1;
    }

    fdwrite = open(FIFO_WRITE_PATH, O_WRONLY);
    if(fdwrite == -1){
        perror("open write fifo");
        return -1;
    }

    printf("UI conectada. Esperando solicitudes...\n");

    while(1){
        int identify;
        r = read(fdread, &identify, sizeof(int));

        if(r <= 0){
            printf("UI desconectada. Cerrando servidor.\n");
            break;
        }

        /* Opcion 1: Busqueda */
        if(identify == 1){
            Query query;
            read(fdread, &query, sizeof(Query));
            printf("Busqueda recibida: titulo='%s' artista='%s'\n",
                   query.title, query.artist);

            Row result;

            /* Simular resultado para "Solitaria" de "Alkilados" */
            if(strcasecmp(query.title,  "solitaria") == 0 &&
               strcasecmp(query.artist, "alkilados") == 0){

                result.id = 1;
                strncpy(result.title,     "Solitaria",   sizeof(result.title));
                strncpy(result.artist,    "Alkilados",   sizeof(result.artist));
                strncpy(result.album,     "Alkilados",   sizeof(result.album));
                strncpy(result.date,      "2017-01-01",  sizeof(result.date));
                strncpy(result.url,       "https://open.spotify.com/track/test",
                                                         sizeof(result.url));
                strncpy(result.explicito, "False",       sizeof(result.explicito));
                result.rank     = 5;
                result.streams  = 5000000;
                result.duration = 195000.0;

                printf("Enviando resultado encontrado.\n");
                write(fdwrite, &result, sizeof(Row));

            } else {
                memset(&result, 0, sizeof(Row));
                result.id = -1;
                printf("Cancion no encontrada. Enviando NA.\n");
                write(fdwrite, &result, sizeof(Row));
            }
        }

        /* Opcion 2: Insercion */
        else if(identify == 2){
            Row new_row;
            read(fdread, &new_row, sizeof(Row));

            printf("Insercion recibida:\n");
            printf("  ID:      %d\n",   new_row.id);
            printf("  Titulo:  %s\n",   new_row.title);
            printf("  Artista: %s\n",   new_row.artist);
            printf("  Album:   %s\n",   new_row.album);
            printf("  Fecha:   %s\n",   new_row.date);
            printf("  Streams: %lld\n", new_row.streams);

            int confirm = 1;
            write(fdwrite, &confirm, sizeof(int));
            printf("Confirmacion de insercion enviada.\n");
        }

        else {
            printf("Comando desconocido: %d\n", identify);
        }
    }

    close(fdread);
    close(fdwrite);
    unlink(FIFO_READ_PATH);
    unlink(FIFO_WRITE_PATH);

    return 0;
}
