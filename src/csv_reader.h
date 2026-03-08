#ifndef CSV_READER_H
#define CSV_READER_H

typedef struct row{
    int id;
    char title[512];
    short rank;
    char date[16];
    char artist[2048];
    char url[64];
    short streams;
    char album[512];
    int duration;
    char explicito[8];
} 



#endif