#include "hash.h"
#include "csv_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int create_hash_table(Hash_node **hash_table){
    for(int i = 0; i < HASH_TABLE_SIZE; i++){
        hash_table[i] = NULL;
    }
    return 0;
}

int normalize_string(char *input, char *output, size_t size){

    if(input == NULL || output == NULL || size == 0){
        return -1;
    }

    int i;

    for(i = 0; input[i] != '\0' && i < size - 1; i++){
        output[i] = tolower(input[i]);
    }

    output[i] = '\0';
    return 0;
}

unsigned int hash(char *title){
    char normalized_title[512];

    if(normalize_string(title, normalized_title, sizeof(normalized_title)) != 0){
        fprintf(stderr, "Error normalizando el titulo: %s\n", title);
        return -1;
    }    

    unsigned int hash_value = 5381;             // Se emplea el algoritmo djb2 para generar el hash de la cadena de texto, con unsigned para no desborar el int.
    int i;                   

    for(i = 0; normalized_title[i] != '\0'; i++){
        hash_value = hash_value * 33 + normalized_title[i];
    }

    return hash_value % HASH_TABLE_SIZE;
}

int insert_node(Hash_node **hash_table, char *title, char *artist, long offset){
    char normalized_title[512];

    if(normalize_string(title, normalized_title, sizeof(normalized_title)) != 0){
        fprintf(stderr, "Error normalizando el titulo: %s\n", title);
        return -1;
    }

    char normalized_artist[2048];

    if(normalize_string(artist, normalized_artist, sizeof(normalized_artist)) != 0){
        fprintf(stderr, "Error normalizando el artista: %s\n", artist);
        return -1;
    }

    Hash_node *new_node = malloc(sizeof(Hash_node));
    
    if(new_node == NULL){
        perror("Error al crear el nuevo nodo.\n");
        return -1;
    }
    
    strncpy(new_node->title, normalized_title, sizeof(new_node->title));
    strncpy(new_node->artist, normalized_artist, sizeof(new_node->artist));
    new_node->offset = offset;
    
    unsigned int index = hash(normalized_title);
    
    Hash_node *nodo_indice = hash_table[index];
    
    if(nodo_indice == NULL){
        hash_table[index] = new_node;
        return 0;
    }
    
    while(nodo_indice->next != NULL){
        
        if(strcmp(nodo_indice->title, new_node->title) == 0 &&
           strcmp(nodo_indice->artist, new_node->artist) == 0
        ){
            return 1;
        }
        
        nodo_indice = nodo_indice->next;
    }
    
    if(strcmp(nodo_indice->title, new_node->title) == 0 &&
       strcmp(nodo_indice->artist, new_node->artist) == 0
    ){
        return 1;
    }
    
    nodo_indice->next = new_node;
    return 0;
    
}

long search_node(Hash_node **table, char *title, char *artist){
    char normalized_title[512];

    if(normalize_string(title, normalized_title, sizeof(normalized_title)) != 0){
        fprintf(stderr, "Error normalizando el titulo: %s\n", title);
        return -1;
    }

    char normalized_artist[2048];

    if(normalize_string(artist, normalized_artist, sizeof(normalized_artist)) != 0){
        fprintf(stderr, "Error normalizando el artista: %s\n", artist);
        return -1;
    }
    
    unsigned int index = hash(normalized_title);
    
    Hash_node *nodo_indice = table[index];
    
    while(nodo_indice != NULL){
        
        if(strcmp(nodo_indice->title, normalized_title) == 0 &&
           strcmp(nodo_indice->artist, normalized_artist) == 0 
        ){
            return nodo_indice->offset;
        }
        
        nodo_indice = nodo_indice->next;
    }
    
    return -1;
}

