CC = gcc
CFLAGS = -Wall -Wextra -g
SRC = src

all: hash_process ui_process

hash_process: $(SRC)/hash_process.c $(SRC)/hash.c $(SRC)/csv_reader.c $(SRC)/utils.c
	$(CC) $(CFLAGS) -o $@ $^

ui_process: $(SRC)/ui_process.c $(SRC)/hash.c $(SRC)/csv_reader.c $(SRC)/utils.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f hash_process ui_process
