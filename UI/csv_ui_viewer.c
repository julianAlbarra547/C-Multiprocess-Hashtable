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
 
#define INPUT_BUFFER_SIZE 1024                          /* Limitamos el tamano del Buffer para nuestro caso particular */
#define DEFAULT_FIFO_TO_HASH "/tmp/csv_ui_to_hash"      /* Ruta por defecto del FIFO de escritura (UI -> Hash Server) */
#define DEFAULT_FIFO_FROM_HASH "/tmp/csv_hash_to_ui"    /* Ruta por defecto del FIFO de lectura  (Hash Server -> UI) */
#define RESPONSE_TIMEOUT_SEC 2                          /* Fijamos un tiempo limite de espera de respuesta en segundos */
 
// Esta estructura nos permite agrupar todo el estado de la conexion FIFO, incluyendo descriptores de archivo, stream de escritura, rutas y bandera de conexion
typedef struct {
  int write_fd;                       // Descriptor de archivo para escritura hacia el hash server 
  int read_fd;                        // Descriptor de archivo para lectura desde el hash server 
  FILE *write_stream;                 // Stream sobre write_fd para usar fprintf y buffer de linea 
  char write_path[INPUT_BUFFER_SIZE]; // Ruta del FIFO de escritura 
  char read_path[INPUT_BUFFER_SIZE];  // Ruta del FIFO de lectura 
  int connected;                      // Bandera: 1 si los FIFOs estan abiertos, 0 si no 
} FifoClient;
 
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
  if (client->write_stream) {
    fclose(client->write_stream);                                                                           // fclose cierra el fd subyacente write_fd
    client->write_stream = NULL;
  } else if (client->write_fd >= 0) {
    close(client->write_fd);                                                                                // Si no hay stream, cierra el fd directamente 
  }
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
 
  FILE *write_stream = fdopen(write_fd, "w");                                                              // Introduce write_fd en un FILE* para usar fprintf, aunque no es necesario pero para simplificar 
  if (!write_stream) {
    perror("fdopen");
    close(write_fd);
    close(read_fd);
    return 0;
  }
 
  setvbuf(write_stream, NULL, _IOLBF, 0);                                                                 // Configuramos buffer de linea; el stream se vacia automaticamente con cada '\n' 
 
  // Guardamos todo en la estructura del cliente 
  
  client->write_fd     = write_fd;
  client->read_fd      = read_fd;
  client->write_stream = write_stream;
  client->connected    = 1;
 
  // Copiamos las rutas de forma segura para evitar desbordamiento de buffer 
  
  strncpy(client->write_path, write_path, sizeof(client->write_path) - 1);
  client->write_path[sizeof(client->write_path) - 1] = '\0';
  strncpy(client->read_path, read_path, sizeof(client->read_path) - 1);
  client->read_path[sizeof(client->read_path) - 1] = '\0';
 
  return 1;  // Exito!
}
 
// read_response() espera una respuesta del hash server a traves del FIFO de lectura.
void read_response(FifoClient *client) {
  if (!client || client->read_fd < 0) {
    printf("No hubo respuesta del cliente.\n");
    return;
  }
 
  fd_set read_set;
  FD_ZERO(&read_set);                                                       // Inicializa el conjunto de descriptores vacio 
  FD_SET(client->read_fd, &read_set);                                       // Agrega el fd de lectura al conjunto 
 
  // Tiempo maximo de espera 
  struct timeval timeout;
  timeout.tv_sec  = RESPONSE_TIMEOUT_SEC;                                   // Timeout de RESPONSE_TIMEOUT_SEC segundos
  timeout.tv_usec = 0;
 
  int ready = select(client->read_fd + 1, &read_set, NULL, NULL, &timeout); // select() bloquea hasta que haya datos disponibles o se agote el timeout
  if (ready == 0) {
    printf("No hubo respuesta (timeout).\n");   // Se agoto el tiempo de espera 
    return;
  }
  if (ready < 0) {
    perror("seleccione");                       // Error interno de select() 
    return;
  }
 
  // Lectura de los datos disponibles del FIFO 
  char buffer[INPUT_BUFFER_SIZE];
  ssize_t bytes = read(client->read_fd, buffer, sizeof(buffer) - 1);
  if (bytes <= 0) {
    if (bytes == 0) {
      printf("Hash server cerro la llamada por FIFO.\n");       // EOF: el servidor cerro su extremo 
    } else {
      perror("Lectura: ");                                      // Error en lectura 
    }
    return;
  }
  buffer[bytes] = '\0';               
  printf("Respuesta: %s", buffer);
  if (buffer[bytes - 1] != '\n') printf("\n");                  // Asegura salto de linea al final 
}
 
/* send_command() envia un comando al hash server a traves del FIFO de escritura, con el formato: "COMANDO|payload\n" o "COMANDO\n" si no hay payload.
   Luego espera y muestra la respuesta con read_response(). */
void send_command(FifoClient *client, const char *command, const char *payload) {
  if (!client || !client->connected || !client->write_stream) {
    printf("FIFO no conectada.\n");                              // Verifica que la conexion este activa */
    return;
  }
  if (!command) return;                                          // Se evita el comando nulo 
 
  if (payload && payload[0]) {
    fprintf(client->write_stream, "%s|%s\n", command, payload);  // Enviamos comando con payload: "COMANDO|dato\n" */
  } else {
    fprintf(client->write_stream, "%s\n", command);              // Envia solo el comando sin payload: "COMANDO\n" 
  }
  fflush(client->write_stream);                                  // Fuerza el vaciado del buffer hacia el FIFO 
 
  read_response(client);                                         // Espera y muestra la respuesta del servidor 
}
 
void print_menu(void) {                                          // Imprime el menu principal de opciones en pantalla.
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
 
      case 2: {                                             // Cargar un archivo CSV enviando su ruta al hash server
        char path[INPUT_BUFFER_SIZE];
        prompt("Direccion del archivo CSV: ", path, sizeof(path));
        if (path[0] == '\0') {
          printf("No se dio ninguna direccion.\n");
          break;
        }
        send_command(&client, "CARGAR", path);              // Envia: "CARGAR|/ruta/al/archivo.csv" 
        break;
      }
 
      case 3: {                                             //  Buscar un registro por termino en la tabla hash
        char term[INPUT_BUFFER_SIZE];
        prompt("Buscar por termino: ", term, sizeof(term));
        send_command(&client, "BUSCAR", term);              // Envia: "BUSCAR|termino" 
        break;
      }

      case 4: {                                             // Agregar un nuevo registro al CSV/hash
        char record[INPUT_BUFFER_SIZE];
        prompt("Columnas del CSV  (separadas por coma): ", record, sizeof(record));
        send_command(&client, "ADD", record);               // Envia: "ADD|col1,col2,col3" 
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
