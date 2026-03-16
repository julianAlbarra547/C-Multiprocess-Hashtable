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

void print_menu(){                                       // Imprime el menu principal de opciones en pantalla.
    printf("\n Bienvenido! Seleccione una opcion:\n");
    printf("1) Buscar canción.\n");
    printf("2) Agregar registro.\n");
    printf("3) Salir\n");
    printf("Elegir opcion: ");
}

void option1(int fdwrite, int fdread){                                  // Opcion 1: Buscar cancion. Solicita al usuario el titulo y artista de la cancion a buscar, valida la entrada y envia la consulta al servidor.
    char title[512];
    char artist[1024];
    Query query;
    short invalid = 1;

    while(invalid){
        printf("Opcion 1 seleccionada.\n");
        printf("Ingrese el titulo de la cancion (para finalizar la escritura presione enter): ");
        gets(title, sizeof(title), stdin);
        
        if(strncmp(title, "\n", 1) == 0){
            printf("Titulo no puede estar vacio. Intente nuevamente.\n");
            continue;
        }

        if(strlen(title) >= sizeof(query.title)){
            printf("Titulo demasiado largo. Intente nuevamente.\n");
            continue;
        }

        if(strncmp(title, "\0", 1) != 0){
            printf("Titulo valido.\n");
            continue;
        }

        invalid = 0;
    }

    invalid = 1;

    while(invalid){
        printf("Ingrese el artista de la cancion (para finalizar la escritura presione enter): ");
        gets(artist, sizeof(artist), stdin);

        if(strlen(artist) >= sizeof(query.artist)){
            printf("Artista demasiado largo. Intente nuevamente.\n");
            continue;
        }

        invalid = 0;
    }

    strncpy(query.title, title, sizeof(query.title));
    strncpy(query.artist, artist, sizeof(query.artist));

    write(fdwrite, &query, sizeof(Query));

    Row result;
    read(fdread, &result, sizeof(Row));

    if(result.id == -1){
        printf("Cancion no encontrada.\n");
    } else {
        print_row(&result);
    }
}

void option2(){             // Opcion 2: Agregar registro. Solicita al usuario los datos de la cancion a agregar, valida la entrada y envia la consulta al servidor.
    
    
    printf("Opcion 2 seleccionada.\n");
}

int main(){
    int fdwrite, fdread;
    short option;
    short start = 1;

    fdwrite = open(FIFO_CLIENT_PATH, O_WRONLY);
    if (fdwrite == -1) {
        perror("Open write fifo");
        return -1;
    }

    fdread = open(FIFO_SERVER_PATH, O_RDONLY);
    if (fdread == -1) {
        perror("Open read fifo");
        return -1;
    }

    while(start){
        print_menu();
        scanf("%hd", &option);

        switch (option){
            case 1:
                option1(fdwrite, fdread);
                continue;;
            case 2:
                printf("Opcion 2 seleccionada.\n");
                break;
            case 3:
                printf("Saliendo del programa...\n");
                start = 0;
                continue;
            default:
                printf("Opcion no valida. Intente nuevamente.\n");
        }
    }

    close(fdwrite);
    close(fdread);
}