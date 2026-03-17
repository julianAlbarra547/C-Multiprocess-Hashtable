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

void trim(char *text) {                                                                                    // Se evita trabajar con el puntero nulo 
  if (!text){
   return;                                                                                        
  }
 
  char *start = text;
  while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) {                 // Avanza 'start' mientras haya espacios en blanco al inicio
   start++;        
  }
 
  char *end = text + strlen(text);
  while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' || end[-1] == '\r')){         // Retrocede 'end' mientras haya espacios en blanco al final
   end--;   
  }
 
  size_t len = (size_t)(end - start);                                                                       // Longitud del string ya trimeado
  if (start != text){                                                                                       // Si hubo espacios al inicio, mueve el contenido al principio del buffer                                    
   memmove(text, start, len);    
  }
 
  text[len] = '\0';                   
}

void option1(int fdwrite, int fdread){                                  // Opcion 1: Buscar cancion. Solicita al usuario el titulo y artista de la cancion a buscar, valida la entrada y envia la consulta al servidor.
    char title[512];
    char artist[2048];
    Query query;
    int indentify = 1;
    short invalid = 1;

    while(invalid){
        printf("Opcion 1 seleccionada.\n");
        printf("Ingrese el titulo de la cancion (para finalizar la escritura presione enter): ");
        printf("Nota: El titulo no puede estar vacio.\n");
        printf("Nota: El titulo no puede exceder los 512 caracteres.\n");
        printf("Nota: El titulo no puede ser solo espacios en blanco.\n");
        printf("Nota: Para regresar al menu principal, ingrese un 0 en blanco y presione enter.\n");
        fgets(title, sizeof(title), stdin);
        trim(title);

        if(strlen(title) == 0){  
            printf("Titulo no puede estar vacio. Intente nuevamente.\n");
            continue;
        }

        if(strlen(title) >= sizeof(query.title)){
            printf("Titulo demasiado largo. Intente nuevamente.\n");
            continue;
        }

        if(strncmp(title, "0", 1) == 0){
            printf("Regresando al menu principal...\n");
            return;
        }

        invalid = 0;
    }

    invalid = 1;

    while(invalid){
        printf("Ingrese el artista de la cancion (para finalizar la escritura presione enter): ");
        fgets(artist, sizeof(artist), stdin);
        trim(artist);

        if(strlen(artist) == 0){
            printf("Artista no puede estar vacio. Intente nuevamente.\n");
            continue;
        }
        
        if(strlen(artist) >= sizeof(query.artist)){
            printf("Artista demasiado largo. Intente nuevamente.\n");
            continue;
        }

        invalid = 0;
    }

    strncpy(query.title, title, sizeof(query.title));
    strncpy(query.artist, artist, sizeof(query.artist));

    write(fdwrite, &indentify, sizeof(int));
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
    Row new_row;
    

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