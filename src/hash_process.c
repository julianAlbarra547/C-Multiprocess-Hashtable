#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common.h"

#define FIFO_READ_PATH "/tmp/client_fifo"
#define FIFO_WRITE_PATH "/tmp/server_fifo"

int main(){
    Query query;
    int r, fdread, fdwrite;

    r = mkfifo(FIFO_READ_PATH, 0666);
    if (r == -1) {
        perror("mkfifo read");
        return -1;
    }

    r = mkfifo(FIFO_WRITE_PATH, 0666);
    if (r == -1) {
        perror("mkfifo write");
        return -1;
    }

    fdread = open(FIFO_READ_PATH, O_RDONLY);
    if (fdread == -1) {
        perror("Open read fifo");
        return -1;
    }

    fdwrite = open(FIFO_WRITE_PATH, O_WRONLY);
    if (fdwrite == -1) {
        perror("Open write fifo");
        return -1;
    }

    return 0;
}