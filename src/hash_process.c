#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common.h"
#include "csv_reader.h"
#include <fcntl.h>
#include <string.h>
#include <errno.h>


int main(){
    Query query;
    int r, fdread, fdwrite;

    r = mkfifo(FIFO_CLIENT_PATH, 0666);
    if (r == -1) {
        perror("mkfifo read");
        return -1;
    }

    r = mkfifo(FIFO_SERVER_PATH, 0666);
    if (r == -1) {
        perror("mkfifo write");
        return -1;
    }

    fdread = open(FIFO_CLIENT_PATH, O_RDONLY);
    if (fdread == -1) {
        perror("Open read fifo");
        return -1;
    }

    fdwrite = open(FIFO_SERVER_PATH, O_WRONLY);
    if (fdwrite == -1) {
        perror("Open write fifo");
        return -1;
    }

    while(1){
        
        if (read(fdread, &query, sizeof(Query)) <= 0) {
            perror("Read from fifo");
            continue;
        }

        //printf("Received query: Title='%s', Artist='%s'\n", query.title, query.artist);

        

    }


    return 0;
}