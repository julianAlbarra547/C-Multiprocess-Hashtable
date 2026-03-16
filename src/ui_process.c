#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common.h"
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

void option1(){
    char title[512];
    char artist[1024];


    printf("Opcion 1 seleccionada.\n");

}

int main(){
    Query query;
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
                break;
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