#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define INPUT_BUFFER_SIZE 1024
#define DEFAULT_FIFO_TO_HASH "/tmp/csv_ui_to_hash"
#define DEFAULT_FIFO_FROM_HASH "/tmp/csv_hash_to_ui"
#define RESPONSE_TIMEOUT_SEC 2

typedef struct {
  int write_fd;
  int read_fd;
  FILE *write_stream;
  char write_path[INPUT_BUFFER_SIZE];
  char read_path[INPUT_BUFFER_SIZE];
  int connected;
} FifoClient;

static void trim_in_place(char *text) {
  if (!text) return;
  char *start = text;
  while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) start++;
  char *end = text + strlen(text);
  while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' || end[-1] == '\r')) end--;
  size_t len = (size_t)(end - start);
  if (start != text) memmove(text, start, len);
  text[len] = '\0';
}

static void prompt_line(const char *label, char *out, size_t out_size) {
  printf("%s", label);
  if (!fgets(out, out_size, stdin)) {
    out[0] = '\0';
    return;
  }
  out[strcspn(out, "\r\n")] = '\0';
  trim_in_place(out);
}

static void close_fifos(FifoClient *client) {
  if (!client) return;
  if (client->write_stream) {
    fclose(client->write_stream);
    client->write_stream = NULL;
  } else if (client->write_fd >= 0) {
    close(client->write_fd);
  }
  if (client->read_fd >= 0) {
    close(client->read_fd);
  }
  client->write_fd = -1;
  client->read_fd = -1;
  client->connected = 0;
}

static int connect_fifos(FifoClient *client, const char *write_path, const char *read_path) {
  if (!client || !write_path || !read_path) return 0;

  int write_fd = open(write_path, O_WRONLY | O_NONBLOCK);
  if (write_fd < 0) {
    if (errno == ENXIO) {
      printf("No hay lector en el FIFO actualmente. Ejecute el hash server primero.\n");
    } else if (errno == ENOENT) {
      printf("FIFO de escritura no existente. Crearlo con: mkfifo %s\n", write_path);
    } else {
      perror("abrir escritura FIFO");
    }
    return 0;
  }

  int read_fd = open(read_path, O_RDONLY | O_NONBLOCK);
  if (read_fd < 0) {
    if (errno == ENOENT) {
      printf("FIFO de lectura no existente. Crearlo con: mkfifo %s\n", read_path);
    } else {
      perror("abrir lectura FIFO");
    }
    close(write_fd);
    return 0;
  }

  FILE *write_stream = fdopen(write_fd, "w");
  if (!write_stream) {
    perror("fdopen");
    close(write_fd);
    close(read_fd);
    return 0;
  }

  setvbuf(write_stream, NULL, _IOLBF, 0);

  client->write_fd = write_fd;
  client->read_fd = read_fd;
  client->write_stream = write_stream;
  client->connected = 1;
  strncpy(client->write_path, write_path, sizeof(client->write_path) - 1);
  client->write_path[sizeof(client->write_path) - 1] = '\0';
  strncpy(client->read_path, read_path, sizeof(client->read_path) - 1);
  client->read_path[sizeof(client->read_path) - 1] = '\0';
  return 1;
}

static void read_response(FifoClient *client) {
  if (!client || client->read_fd < 0) {
    printf("No hubo respuesta del cliente.\n");
    return;
  }

  fd_set read_set;
  FD_ZERO(&read_set);
  FD_SET(client->read_fd, &read_set);

  struct timeval timeout;
  timeout.tv_sec = RESPONSE_TIMEOUT_SEC;
  timeout.tv_usec = 0;

  int ready = select(client->read_fd + 1, &read_set, NULL, NULL, &timeout);
  if (ready == 0) {
    printf("No hubo respuesta (timeout).\n");
    return;
  }
  if (ready < 0) {
    perror("seleccione");
    return;
  }

  char buffer[INPUT_BUFFER_SIZE];
  ssize_t bytes = read(client->read_fd, buffer, sizeof(buffer) - 1);
  if (bytes <= 0) {
    if (bytes == 0) {
      printf("Hash server cerro la llamada por FIFO.\n");
    } else {
      perror("Lectura: ");
    }
    return;
  }
  buffer[bytes] = '\0';
  printf("Respuesta: %s", buffer);
  if (buffer[bytes - 1] != '\n') printf("\n");
}

static void send_command(FifoClient *client, const char *command, const char *payload) {
  if (!client || !client->connected || !client->write_stream) {
    printf("FIFO no conectada.\n");
    return;
  }
  if (!command) return;

  if (payload && payload[0]) {
    fprintf(client->write_stream, "%s|%s\n", command, payload);
  } else {
    fprintf(client->write_stream, "%s\n", command);
  }
  fflush(client->write_stream);

  read_response(client);
}

static void print_menu(void) {
  printf("\n Bienvenido! Seleccione una opcion:\n");
  printf("1) Conectarse a los FIFOs\n");
  printf("2) Cargar CSV (envie direccion)\n");
  printf("3) Busqueda\n");
  printf("4) Agregar registro\n");
  printf("5) Cerrar FIFOs\n");
  printf("0) Salir\n");
  printf("Elegir opcion: ");
}

int main(void) {
  FifoClient client;
  memset(&client, 0, sizeof(client));
  client.write_fd = -1;
  client.read_fd = -1;

  char input[INPUT_BUFFER_SIZE];
  while (1) {
    print_menu();
    if (!fgets(input, sizeof(input), stdin)) break;
    int option = atoi(input);

    switch (option) {
      case 1: {
        if (client.connected) {
          printf("Conexion previamente establecida. Escribir FIFO: %s | Leer FIFO: %s\n", client.write_path, client.read_path);
          break;
        }
        char write_path[INPUT_BUFFER_SIZE];
        char read_path[INPUT_BUFFER_SIZE];
        prompt_line("FIFO hacia el hash (default /tmp/csv_ui_to_hash): ", write_path, sizeof(write_path));
        prompt_line("FIFO desde el hash (default /tmp/csv_hash_to_ui): ", read_path, sizeof(read_path));
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
          printf("Conectado. Escribir FIFO: %s | Leer FIFO: %s\n", client.write_path, client.read_path);
        }
        break;
      }
      case 2: {
        char path[INPUT_BUFFER_SIZE];
        prompt_line("Direccion del archivo CSV: ", path, sizeof(path));
        if (path[0] == '\0') {
          printf("No se dio ninguna direccion.\n");
          break;
        }
        send_command(&client, "CARGAR", path);
        break;
      }
      case 3: {
        char term[INPUT_BUFFER_SIZE];
        prompt_line("Buscar por termino: ", term, sizeof(term));
        send_command(&client, "BUSCAR", term);
        break;
      }
      case 4: {
        char record[INPUT_BUFFER_SIZE];
        prompt_line("Columnas del CSV  (separadas por coma): ", record, sizeof(record));
        send_command(&client, "ADD", record);
        break;
      }
      case 5:
        if (!client.connected) {
          printf("FIFOs no conectadas.\n");
        } else {
          close_fifos(&client);
          printf("FIFOs cerradas.\n");
        }
        break;
      case 0:
        if (client.connected) close_fifos(&client);
        return 0;
      default:
        printf("Opcion desconocida.\n");
        break;
    }
  }

  if (client.connected) close_fifos(&client);
  return 0;
}
