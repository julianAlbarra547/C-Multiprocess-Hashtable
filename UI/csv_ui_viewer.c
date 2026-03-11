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
      printf("No reader on FIFO yet. Start the hash server first.\n");
    } else if (errno == ENOENT) {
      printf("Write FIFO does not exist. Create it with: mkfifo %s\n", write_path);
    } else {
      perror("open write FIFO");
    }
    return 0;
  }

  int read_fd = open(read_path, O_RDONLY | O_NONBLOCK);
  if (read_fd < 0) {
    if (errno == ENOENT) {
      printf("Read FIFO does not exist. Create it with: mkfifo %s\n", read_path);
    } else {
      perror("open read FIFO");
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
    printf("No response channel.\n");
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
    printf("No response (timeout).\n");
    return;
  }
  if (ready < 0) {
    perror("select");
    return;
  }

  char buffer[INPUT_BUFFER_SIZE];
  ssize_t bytes = read(client->read_fd, buffer, sizeof(buffer) - 1);
  if (bytes <= 0) {
    if (bytes == 0) {
      printf("Hash server closed the response FIFO.\n");
    } else {
      perror("read");
    }
    return;
  }
  buffer[bytes] = '\0';
  printf("Response: %s", buffer);
  if (buffer[bytes - 1] != '\n') printf("\n");
}

static void send_command(FifoClient *client, const char *command, const char *payload) {
  if (!client || !client->connected || !client->write_stream) {
    printf("FIFO not connected.\n");
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
  printf("\nCSV UI Console Menu\n");
  printf("1) Connect to FIFOs\n");
  printf("2) Load CSV (send path)\n");
  printf("3) Search\n");
  printf("4) Add register\n");
  printf("5) Close FIFOs\n");
  printf("0) Exit\n");
  printf("Select option: ");
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
          printf("Already connected. Write FIFO: %s | Read FIFO: %s\n", client.write_path, client.read_path);
          break;
        }
        char write_path[INPUT_BUFFER_SIZE];
        char read_path[INPUT_BUFFER_SIZE];
        prompt_line("FIFO to hash (default /tmp/csv_ui_to_hash): ", write_path, sizeof(write_path));
        prompt_line("FIFO from hash (default /tmp/csv_hash_to_ui): ", read_path, sizeof(read_path));
        if (write_path[0] == '\0') {
          strncpy(write_path, DEFAULT_FIFO_TO_HASH, sizeof(write_path) - 1);
          write_path[sizeof(write_path) - 1] = '\0';
        }
        if (read_path[0] == '\0') {
          strncpy(read_path, DEFAULT_FIFO_FROM_HASH, sizeof(read_path) - 1);
          read_path[sizeof(read_path) - 1] = '\0';
        }
        if (!connect_fifos(&client, write_path, read_path)) {
          printf("Failed to connect to FIFOs.\n");
        } else {
          printf("Connected. Write FIFO: %s | Read FIFO: %s\n", client.write_path, client.read_path);
        }
        break;
      }
      case 2: {
        char path[INPUT_BUFFER_SIZE];
        prompt_line("CSV file path: ", path, sizeof(path));
        if (path[0] == '\0') {
          printf("No path provided.\n");
          break;
        }
        send_command(&client, "LOAD", path);
        break;
      }
      case 3: {
        char term[INPUT_BUFFER_SIZE];
        prompt_line("Search term: ", term, sizeof(term));
        send_command(&client, "SEARCH", term);
        break;
      }
      case 4: {
        char record[INPUT_BUFFER_SIZE];
        prompt_line("CSV row (comma-separated): ", record, sizeof(record));
        send_command(&client, "ADD", record);
        break;
      }
      case 5:
        if (!client.connected) {
          printf("FIFOs not connected.\n");
        } else {
          close_fifos(&client);
          printf("FIFOs closed.\n");
        }
        break;
      case 0:
        if (client.connected) close_fifos(&client);
        return 0;
      default:
        printf("Unknown option.\n");
        break;
    }
  }

  if (client.connected) close_fifos(&client);
  return 0;
}
