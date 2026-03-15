#ifndef HASH_H
#define HASH_H

#include <stdio.h>
#include <stdlib.h>

#define HASH_TABLE_SIZE 10000
#define TABLE_IDX "spotify.idx"
#define ENTRIES_BIN "spotify_entries.bin"

typedef struct hash_node{
    char title[512];
    char artist[2048];
    long offset;
    long next_entry;
} Hash_node;

unsigned int hash(char *title);
int build_index(const char *csv_path, const char *idx_path, const char *entries_path);
long search_node(long *table, FILE *entries, char *title, char *artist);
int load_table(const char *idx_path, long *table);

#endif
