#include <errno.h>          // Manejo de errores del sistema (errno, perror)
#include <fcntl.h>          // Control de descriptores de archivo (open, O_WRONLY, O_NONBLOCK, etc.)
#include <stdbool.h>        // Importacion tipo booleano (bool, true, false)
#include <stdio.h>          
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>     // Multiplexado de I/O sin bloqueo
#include <sys/stat.h>       // Constantes de permisos de archivos (S_IRUSR, etc.)
#include <sys/types.h>      // Tipos de datos del sistema (ssize_t, size_t, etc.)
#include <unistd.h>         // Llamadas al sistema POSIX (read, write, close, open)

#include "../src/ipc_protocol.h"
 
#define INPUT_BUFFER_SIZE 1024                          /* Limitamos el tamano del Buffer para nuestro caso particular */
#define DEFAULT_FIFO_TO_HASH "/tmp/csv_ui_to_hash"      /* Ruta por defecto del FIFO de escritura (UI -> Hash Server) */
#define DEFAULT_FIFO_FROM_HASH "/tmp/csv_hash_to_ui"    /* Ruta por defecto del FIFO de lectura  (Hash Server -> UI) */
#define RESPONSE_TIMEOUT_SEC 2                          /* Fijamos un tiempo limite de espera de respuesta en segundos */
 
// Esta estructura nos permite agrupar todo el estado de la conexion FIFO, incluyendo descriptores de archivo, stream de escritura, rutas y bandera de conexion
typedef struct {
  int write_fd;                       // Descriptor de archivo para escritura hacia el hash server 
  int read_fd;                        // Descriptor de archivo para lectura desde el hash server 
  char write_path[INPUT_BUFFER_SIZE]; // Ruta del FIFO de escritura 
  char read_path[INPUT_BUFFER_SIZE];  // Ruta del FIFO de lectura 
  int connected;                      // Bandera: 1 si los FIFOs estan abiertos, 0 si no 
} FifoClient;

static int write_all(int fd, const void *buffer, size_t len) {
  const unsigned char *ptr = (const unsigned char *)buffer;
  size_t total = 0;
  while (total < len) {
    ssize_t written = write(fd, ptr + total, len - total);
    if (written < 0) {
      if (errno == EINTR) continue;
      return 0;
    }
    total += (size_t)written;
  }
  return 1;
}

static int read_all_with_timeout(int fd, void *buffer, size_t len, int timeout_sec) {
  unsigned char *ptr = (unsigned char *)buffer;
  size_t total = 0;
  while (total < len) {
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);

    struct timeval timeout;
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;

    int ready = select(fd + 1, &read_set, NULL, NULL, &timeout);
    if (ready == 0) return 0;
    if (ready < 0) {
      if (errno == EINTR) continue;
      return 0;
    }

    ssize_t bytes = read(fd, ptr + total, len - total);
    if (bytes < 0) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
      return 0;
    }
    if (bytes == 0) return 0;
    total += (size_t)bytes;
  }
  return 1;
}

static int ipc_send(int fd, uint16_t type, const void *payload, uint32_t payload_len) {
  IpcHeader header;
  header.magic = IPC_MAGIC;
  header.version = IPC_VERSION;
  header.type = type;
  header.payload_len = payload_len;

  if (!write_all(fd, &header, sizeof(header))) return 0;
  if (payload_len == 0) return 1;
  return write_all(fd, payload, payload_len);
}

static int ipc_recv(int fd, IpcHeader *header_out, void *payload, size_t payload_capacity) {
  if (!read_all_with_timeout(fd, header_out, sizeof(*header_out), RESPONSE_TIMEOUT_SEC)) return 0;
  if (header_out->magic != IPC_MAGIC || header_out->version != IPC_VERSION) return 0;
  if (header_out->payload_len > payload_capacity) return 0;
  if (header_out->payload_len == 0) return 1;
  return read_all_with_timeout(fd, payload, header_out->payload_len, RESPONSE_TIMEOUT_SEC);
}
 
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
 
// close_fifos() cierra de forma segura el stream y los descriptores de archivo del cliente FIFO y restablece los campos a su estado inicial (desconectado).
void close_fifos(FifoClient *client) {
  if (!client) return;                                                                                      // Se evita el puntero nulo 
  if (client->write_fd >= 0) close(client->write_fd);
  if (client->read_fd >= 0) {
    close(client->read_fd);                                                                                 // Cierra el descriptor de lectura 
  }
  client->write_fd = -1;                                                                                    // Ahora los descriptores previos son invalidos 
  client->read_fd  = -1;
  client->connected = 0;                                                                                    // Indica que ya no hay conexion activa 
}
 
/* connect_fifos() abre los FIFOs de escritura y lectura en modo no bloqueante y guarda
   los descriptores y el stream en la estructura FifoClient.
   .
   Retorna 1 si la conexion fue exitosa, 0 si fallo.
   
   Diagrama de comunicacion:
   
     UI --(write_fd)--> FIFO_TO_HASH   --> Hash Server
     UI <--(read_fd)--- FIFO_FROM_HASH <-- Hash Server  */

int connect_fifos(FifoClient *client, const char *write_path, const char *read_path) {
  if (!client || !write_path || !read_path) return 0;                                                      // Validamos que el cliente sea correcto
  int write_fd = open(write_path, O_WRONLY | O_NONBLOCK);                                                  // Abrimos el FIFO de escritura en modo no bloqueante (O_NONBLOCK), evita que open() se bloquee si el otro extremo aun no esta listo
  if (write_fd < 0) {                                     
    if (errno == ENXIO) {                                                                                  // Excepcion ENXIO: el FIFO existe pero nadie lo esta leyendo (servidor no iniciado) 
      printf("No hay lector en el FIFO actualmente. Ejecute el hash server primero.\n");
    } else if (errno == ENOENT) {                                                                          // Excepcion NOENT: el archivo FIFO no existe en el sistema de archivos 
      printf("FIFO de escritura no existente. Crearlo con: mkfifo %s\n", write_path);
    } else {
      perror("abrir escritura FIFO");                                                                      // Algun otro error del sistema (interrupciones, bloqueos en el csv, etc) 
    }
    return 0;
  }

  int read_fd = open(read_path, O_RDONLY | O_NONBLOCK);                                                    // Abre el FIFO de lectura en modo no bloqueante 
  if (read_fd < 0) {
    if (errno == ENOENT) {
      printf("FIFO de lectura no existente. Crearlo con: mkfifo %s\n", read_path);
    } else {
      perror("abrir lectura FIFO");
    }
    close(write_fd);                                                                                       // Cierra el fd de escritura antes de retornar 
    return 0;
  }

  // Guardamos todo en la estructura del cliente
  client->write_fd  = write_fd;
  client->read_fd   = read_fd;
  client->connected = 1;
 
  // Copiamos las rutas de forma segura para evitar desbordamiento de buffer 
  
  strncpy(client->write_path, write_path, sizeof(client->write_path) - 1);
  client->write_path[sizeof(client->write_path) - 1] = '\0';
  strncpy(client->read_path, read_path, sizeof(client->read_path) - 1);
  client->read_path[sizeof(client->read_path) - 1] = '\0';
 
  return 1;  // Exito!
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

static void send_query(FifoClient *client, const char *title, const char *artist) {
  if (!client || !client->connected) {
    printf("FIFO no conectada.\n");
    return;
  }

  IpcQuery query;
  memset(&query, 0, sizeof(query));
  strncpy(query.title, title ? title : "", sizeof(query.title) - 1);
  if (artist && artist[0]) {
    query.has_artist = 1;
    strncpy(query.artist, artist, sizeof(query.artist) - 1);
  } else {
    query.has_artist = 0;
  }

  if (!ipc_send(client->write_fd, IPC_MSG_QUERY, &query, (uint32_t)sizeof(query))) {
    printf("Error enviando query.\n");
    return;
  }

  IpcHeader header;
  size_t payload_capacity = sizeof(IpcQueryResp) > sizeof(IpcErrorResp) ? sizeof(IpcQueryResp) : sizeof(IpcErrorResp);
  unsigned char payload[payload_capacity];
  if (!ipc_recv(client->read_fd, &header, payload, sizeof(payload))) {
    printf("No hubo respuesta (timeout).\n");
    return;
  }

  if (header.type == IPC_MSG_QUERY_RESP && header.payload_len == sizeof(IpcQueryResp)) {
    IpcQueryResp resp;
    memcpy(&resp, payload, sizeof(resp));
    if (resp.status == 0) {
      print_row(&resp.row);
    } else if (resp.status == 1) {
      printf("No encontrado.\n");
    } else {
      printf("Error en servidor (status=%d).\n", resp.status);
    }
    return;
  }

  if (header.type == IPC_MSG_ERROR_RESP && header.payload_len == sizeof(IpcErrorResp)) {
    IpcErrorResp err;
    memcpy(&err, payload, sizeof(err));
    printf("Error: %s\n", err.message);
    return;
  }

  printf("Respuesta desconocida.\n");
}

static void send_append(FifoClient *client, const IpcRow *row) {
  if (!client || !client->connected) {
    printf("FIFO no conectada.\n");
    return;
  }

  if (!ipc_send(client->write_fd, IPC_MSG_APPEND, row, (uint32_t)sizeof(*row))) {
    printf("Error enviando append.\n");
    return;
  }

  IpcHeader header;
  unsigned char payload[sizeof(IpcErrorResp)];
  if (!ipc_recv(client->read_fd, &header, payload, sizeof(payload))) {
    printf("No hubo respuesta (timeout).\n");
    return;
  }

  if (header.type == IPC_MSG_APPEND_RESP && header.payload_len == sizeof(IpcAppendResp)) {
    IpcAppendResp resp;
    memcpy(&resp, payload, sizeof(resp));
    if (resp.status == 0) {
      printf("Registro agregado.\n");
    } else {
      printf("Error en servidor (status=%d).\n", resp.status);
    }
    return;
  }

  if (header.type == IPC_MSG_ERROR_RESP && header.payload_len == sizeof(IpcErrorResp)) {
    IpcErrorResp err;
    memcpy(&err, payload, sizeof(err));
    printf("Error: %s\n", err.message);
    return;
  }

  printf("Respuesta desconocida.\n");
}
 
void print_menu(void) {                                          // Imprime el menu principal de opciones en pantalla.
  printf("\n Bienvenido! Seleccione una opcion:\n");
  printf("1) Conectarse a los FIFOs\n");
  printf("2) Buscar (Query)\n");
  printf("3) Agregar registro (Append)\n");
  printf("5) Cerrar FIFOs\n");
  printf("0) Salir\n");
  printf("Elegir opcion: ");
}
 
int main(void) {
  FifoClient client;
  memset(&client, 0, sizeof(client));  // Inicializa toda la estructura en cero 
  client.write_fd = -1;                // -1 indica descriptor invalido (no abierto) 
  client.read_fd  = -1;
 
  char input[INPUT_BUFFER_SIZE];
  while (1) {                          // Procedimiento principal del programa 
    print_menu();
    if (!fgets(input, sizeof(input), stdin)) break;  // Lee el input; sale si hay EOF 
    int option = atoi(input);          // Convierte la entrada a entero 
 
    switch (option) {
 
      case 1: {                                                                                         // Conectar los FIFOs al hash server
        if (client.connected) {
          printf("Conexion previamente establecida. Escribir FIFO: %s | Leer FIFO: %s\n",               // Si se esta conectado, solo informa las rutas activas
                 client.write_path, client.read_path);
          break;
        }
        char write_path[INPUT_BUFFER_SIZE];
        char read_path[INPUT_BUFFER_SIZE];
        prompt("FIFO hacia el hash (default /tmp/csv_ui_to_hash): ", write_path, sizeof(write_path));   // Solicita la ruta de escritura al usuario; si no ingresa nada, usamos las rutas por defecto 
        prompt("FIFO desde el hash (default /tmp/csv_hash_to_ui): ", read_path, sizeof(read_path));     // Solicita la ruta de lectura al usuario; si no ingresa nada, usamos las rutas por defecto 
        if (write_path[0] == '\0') {
          strncpy(write_path, DEFAULT_FIFO_TO_HASH, sizeof(write_path) - 1);
          write_path[sizeof(write_path) - 1] = '\0';
        }
        if (read_path[0] == '\0') {
          strncpy(read_path, DEFAULT_FIFO_FROM_HASH, sizeof(read_path) - 1);
          read_path[sizeof(read_path) - 1] = '\0';
        }
        if (!connect_fifos(&client, write_path, read_path)) {
          printf("Fallo al conectar los FIFOs.\n");
        } else {
          printf("Conectado. Escribir FIFO: %s | Leer FIFO: %s\n",
                 client.write_path, client.read_path);
        }
        break;
      }
 
      case 2: {                                             // Buscar un registro por titulo (y opcionalmente artista)
        char title[INPUT_BUFFER_SIZE];
        char artist[INPUT_BUFFER_SIZE];
        prompt("Title: ", title, sizeof(title));
        prompt("Artist (opcional, Enter para ignorar): ", artist, sizeof(artist));
        if (title[0] == '\0') {
          printf("Title requerido.\n");
          break;
        }
        send_query(&client, title, artist);
        break;
      }

      case 3: {                                             // Agregar un nuevo registro enviando un Row parseado
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
        send_append(&client, &row);
        break;
      }

      case 5:                                               // Cerrar los FIFOs sin salir del programa
        if (!client.connected) {
          printf("FIFOs no conectadas.\n");
        } else {
          close_fifos(&client);
          printf("FIFOs cerradas.\n");
        }
        break;
 
      case 0:                                               // Cerrar FIFOs si estaban abiertos y terminar el programa 
        if (client.connected) close_fifos(&client);
        return 0;
 
      default:
        printf("Opcion desconocida.\n");
        break;
    }
  }
 
  if (client.connected) close_fifos(&client); // Se cierran los FIFOs si el bucle termino por EOF en stdin 
  return 0;
}
