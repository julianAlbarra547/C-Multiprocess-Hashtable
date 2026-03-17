CC = gcc
CFLAGS = -Wall -Wextra -g -Wno-sign-compare -Wno-unused-value
SRC = src
BIN = scripts

all: hash_process ui_process

hash_process: $(SRC)/hash_process.c $(SRC)/hash.c $(SRC)/csv_reader.c $(SRC)/utils.c
	$(CC) $(CFLAGS) -o $(BIN)/$@ $^

ui_process: $(SRC)/ui_process.c $(SRC)/hash.c $(SRC)/csv_reader.c $(SRC)/utils.c
	$(CC) $(CFLAGS) -o $(BIN)/$@ $^

run:
	(cd $(BIN) && ./hash_process) & (cd $(BIN) && ./ui_process)

clean:
	rm -f $(BIN)/hash_process $(BIN)/ui_process