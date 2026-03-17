CC = gcc
CFLAGS = -Wall -Wextra -g
SRC = src
BIN = scripts

all: hash_process ui_process

hash_process: $(SRC)/hash_process.c $(SRC)/hash.c $(SRC)/csv_reader.c $(SRC)/utils.c
	$(CC) $(CFLAGS) -o $(BIN)/$@ $^

ui_process: $(SRC)/ui_process.c $(SRC)/hash.c $(SRC)/csv_reader.c $(SRC)/utils.c
	$(CC) $(CFLAGS) -o $(BIN)/$@ $^

clean:
	rm -f $(BIN)/hash_process $(BIN)/ui_process
