#ifndef HASH_H
#define HASH_H

#include <stdio.h>
#include <stdlib.h>

#define HASH_TABLE_SIZE 10000

typedef struct hash_node{
    char title[512];
    char artist[2048];
    long long offset;
    struct hash_node *next;
} Hash_node;

unsigned int hash(char *title);
int create_hash_table(Hash_node **hash_table);
int insert_node(Hash_node **hash_table, char *title, char *artist, long offset); 
long search_node(Hash_node **table, char *title, char *artist);
int build_index(const char *csv_path, Hash_node **table);
int save_index(Hash_node **table, const char *idx_path);
int load_index(const char *idx_path, Hash_node **table);

#endif
