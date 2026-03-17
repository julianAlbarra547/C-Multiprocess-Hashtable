CC = gcc
CFLAGS = -Wall -g

SERVER = hash_process
CLIENT = ui

all:
$(CC) $(CFLAGS) hash_process.c hash.c csv_reader.c utils.c -o $(SERVER)
$(CC) $(CFLAGS) ui_process.c csv_reader.c utils.c -o $(CLIENT)

run: all
mkfifo /tmp/client_fifo 2>/dev/null || true
mkfifo /tmp/server_fifo 2>/dev/null || true
./$(SERVER) &
sleep 1
./$(CLIENT)

clean:
rm -f $(SERVER) $(CLIENT)
rm -f /tmp/client_fifo /tmp/server_fifo
