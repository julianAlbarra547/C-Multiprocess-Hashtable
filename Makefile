
CC     = gcc
CFLAGS = -Wall -Wextra -g -Isrc -IUI
 
SHARED = src/csv_reader.o src/hash.o
 
all: hash_server csv_ui_viewer build_index
 
hash_server:   $(SHARED) src/hash_server.o
	$(CC) $(CFLAGS) $^ -o $@
 
csv_ui_viewer: $(SHARED) src/ipc_client.o UI/csv_ui_viewer.o
	$(CC) $(CFLAGS) $^ -o $@
 
build_index:   $(SHARED) src/build_index.o
	$(CC) $(CFLAGS) $^ -o $@
 
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
 
clean:
	rm -f src/*.o UI/*.o hash_server csv_ui_viewer build_index