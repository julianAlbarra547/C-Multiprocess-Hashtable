#include <errno.h>          // Manejo de errores del sistema (errno, perror)
#include <stdbool.h>        // Importacion tipo booleano (bool, true, false)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ipc_client.h"
 
#define INPUT_BUFFER_SIZE 1024                          /* Limitamos el tamano del Buffer para nuestro caso particular */
 
// trim() nos permite eliminar espacios, tabulaciones y saltos de linea al inicio y al final para trabajar mejor con el Hash Server, modificandolo desde memoria
void trim(char *text) {
  if (!text) return;                                                                                        // Se evita el puntero nulo 
  char *start = text;
  while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) start++;          // Avanza 'start' mientras haya espacios en blanco al inicio
  char *end = text + strlen(text);
  while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' || end[-1] == '\r')) end--;   // Retrocede 'end' mientras haya espacios en blanco al final
  size_t len = (size_t)(end - start);                                                                       // Longitud del string ya trimeado
  if (start != text) memmove(text, start, len);                                                             // Si hubo espacios al inicio, mueve el contenido al principio del buffer                                                
  text[len] = '\0';                   
}
 
// prompt() pide un input del usuario y aplica trim() para eliminar espacios y saltos de linea.
void prompt(const char *label, char *out, size_t out_size) {
  printf("%s", label);                                                                                      // Pide el input al usuario 
  if (!fgets(out, out_size, stdin)) {                                                                       // Si la lectura de entrada falla, devuelve NULL string.
    out[0] = '\0';
    return;
  }
  out[strcspn(out, "\r\n")] = '\0';                                                                         // Elimina el salto de linea dejado por fgets 
  trim(out);                                                                                                // Aplica trim para limpiar espacios extra 
}
 
static void print_row(const IpcRow *row) {
  printf("\n--- Resultado ---\n");
  printf("ID: %d\n", row->id);
  printf("Title: %s\n", row->title);
  printf("Rank: %d\n", row->rank);
  printf("Date: %s\n", row->date);
  printf("Artist: %s\n", row->artist);
  printf("URL: %s\n", row->url);
  printf("Streams: %lld\n", (long long)row->streams);
  printf("Album: %s\n", row->album);
  printf("Duration: %.0f\n", row->duration);
  printf("Explicit: %s\n", row->explicito);
}

static void send_query(IpcClient *client, const char *title, const char *artist) {
  IpcQuery query;
  memset(&query, 0, sizeof(query));
  strncpy(query.title, title ? title : "", sizeof(query.title) - 1);
  if (artist && artist[0]) {
    query.has_artist = 1;
    strncpy(query.artist, artist, sizeof(query.artist) - 1);
  } else {
    query.has_artist = 0;
  }

  char err[256];
  IpcQueryResp resp;
  if (!ipc_client_query(client, &query, &resp, err, sizeof(err))) {
    printf("Error: %s\n", err[0] ? err : "fallo query");
    return;
  }

  if (resp.status == 0) print_row(&resp.row);
  else if (resp.status == 1) printf("No encontrado.\n");
  else printf("Error en servidor (status=%d).\n", resp.status);
}

static void send_append(IpcClient *client, const IpcRow *row) {
  char err[256];
  IpcAppendResp resp;
  if (!ipc_client_append(client, row, &resp, err, sizeof(err))) {
    printf("Error: %s\n", err[0] ? err : "fallo append");
    return;
  }

  if (resp.status == 0) printf("Registro agregado.\n");
  else printf("Error en servidor (status=%d).\n", resp.status);
}
 
void print_menu(void) {                                          // Imprime el menu principal de opciones en pantalla.
  printf("\n Bienvenido! Seleccione una opcion:\n");
  printf("1) Conectarse (IPC)\n");
  printf("2) Buscar (Query)\n");
  printf("3) Agregar registro (Append)\n");
  printf("5) Cerrar conexion\n");
  printf("0) Salir\n");
  printf("Elegir opcion: ");
}
 
int main(void) {
  IpcClient *client = NULL;
 
  char input[INPUT_BUFFER_SIZE];
  while (1) {                          // Procedimiento principal del programa 
    print_menu();
    if (!fgets(input, sizeof(input), stdin)) break;  // Lee el input; sale si hay EOF 
    int option = atoi(input);          // Convierte la entrada a entero 
 
    switch (option) {
 
      case 1: {                                                                                         // Conectar al servicio IPC (implementacion externa)
        if (client != NULL) {
          printf("Conexion previamente establecida.\n");
          break;
        }
        char endpoint[INPUT_BUFFER_SIZE];
        prompt("Endpoint IPC (Enter para default): ", endpoint, sizeof(endpoint));

        char err[256];
        client = ipc_client_connect(endpoint[0] ? endpoint : NULL, err, sizeof(err));
        if (!client) printf("Fallo al conectar: %s\n", err[0] ? err : "error desconocido");
        else printf("Conectado.\n");
        break;
      }
 
      case 2: {                                             // Buscar un registro por titulo (y opcionalmente artista)
        if (client == NULL) {
          printf("No conectado.\n");
          break;
        }
        char title[INPUT_BUFFER_SIZE];
        char artist[INPUT_BUFFER_SIZE];
        prompt("Title: ", title, sizeof(title));
        prompt("Artist (opcional, Enter para ignorar): ", artist, sizeof(artist));
        if (title[0] == '\0') {
          printf("Title requerido.\n");
          break;
        }
        send_query(client, title, artist);
        break;
      }

      case 3: {                                             // Agregar un nuevo registro enviando un Row parseado
        if (client == NULL) {
          printf("No conectado.\n");
          break;
        }
        IpcRow row;
        memset(&row, 0, sizeof(row));
        char buffer[INPUT_BUFFER_SIZE];

        prompt("ID (int): ", buffer, sizeof(buffer));
        row.id = atoi(buffer);

        prompt("Title: ", row.title, sizeof(row.title));

        prompt("Rank (int): ", buffer, sizeof(buffer));
        row.rank = (int16_t)atoi(buffer);

        prompt("Date (YYYY-MM-DD): ", row.date, sizeof(row.date));
        prompt("Artist: ", row.artist, sizeof(row.artist));
        prompt("URL: ", row.url, sizeof(row.url));

        prompt("Streams (int): ", buffer, sizeof(buffer));
        row.streams = (int64_t)atoll(buffer);

        prompt("Album: ", row.album, sizeof(row.album));

        prompt("Duration_ms (int): ", buffer, sizeof(buffer));
        row.duration = atof(buffer);

        prompt("Explicit (True/False): ", row.explicito, sizeof(row.explicito));

        if (row.title[0] == '\0') {
          printf("Title requerido.\n");
          break;
        }
        send_append(client, &row);
        break;
      }

      case 5:                                               // Cerrar conexion sin salir del programa
        if (client == NULL) printf("No conectado.\n");
        else {
          ipc_client_close(client);
          client = NULL;
          printf("Conexion cerrada.\n");
        }
        break;
 
      case 0:                                               // Cerrar FIFOs si estaban abiertos y terminar el programa 
        if (client != NULL) ipc_client_close(client);
        return 0;
 
      default:
        printf("Opcion desconocida.\n");
        break;
    }
  }
 
  if (client != NULL) ipc_client_close(client); // Se cierra si el bucle termino por EOF en stdin
  return 0;
}
