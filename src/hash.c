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
            free(new_node);
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

int build_index(const char *csv_path, Hash_node **table){
    FILE *file = open_csv(csv_path);
    
    if(file == NULL){
        return -1;
    }

    skip_header(file);

    long offset = ftell(file);

    Row *row;
    row = read_csv(file, offset);

    while(row != NULL){
        int result = insert_node(table, row->title, row->artist, offset);

        if(result == -1){
            fprintf(stderr, "Error al insertar el nodo con titulo: %s y artista: %s\n", row->title, row->artist);
        }

        offset = ftell(file);
        free(row);
        row = read_csv(file, offset);
    }

    close_csv(file);
    return 0;
}

int save_index(Hash_node **table, const char *idx_path){
    FILE *file = fopen(idx_path, "wb");

    if(file == NULL){
        perror("Error al abrir el archivo de indice para escritura.\n");
        return -1;
    }

    int r, size = HASH_TABLE_SIZE;
    r = fwrite(&size, sizeof(size), 1, file);

    if(r != 1){
        perror("Error al escribir el tamaño de la tabla hash en el archivo de indice.\n");
        fclose(file);
        return -1;
    }

    for(int i= 0; i < HASH_TABLE_SIZE; i++){
        int size_bucket = 0;
        Hash_node *aux = table[i];
        
        while(aux != NULL){
            size_bucket++;
            aux = aux->next;
        }
        
        fwrite(&size_bucket, sizeof(int), 1, file);
        
        aux = table[i];
        
        while(aux != NULL){
            
            fwrite(aux->title, sizeof(aux->title), 1, file);
            fwrite(aux->artist, sizeof(aux->artist), 1, file);
            fwrite(&aux->offset, sizeof(aux->offset), 1, file);
            
            aux = aux->next;
        }
    }
    
    fclose(file);

    return 0;
}

int load_index(const char *idx_path, Hash_node **table){
    FILE *file = fopen(idx_path, "rb");
    
    if(file == NULL){
        perror("Error al abrir el archivo idx.\n");
        return -1;
    }

    int verification_size;
    
    fread(&verification_size, sizeof(int), 1, file);
    
    if(verification_size != HASH_TABLE_SIZE){
        perror("Error, tamaños desiguales en la creación del hash.");
        fclose(file);
        return -1;
    }
    
    int amount_nodes;
    
    for(int i = 0; i < HASH_TABLE_SIZE; i++){
        
        fread(&amount_nodes, sizeof(int), 1, file);
        
        Hash_node *last = NULL;
        
        for(int j = 0; j < amount_nodes; j++){
            
            Hash_node *aux = malloc(sizeof(Hash_node));
            fread(aux->title, sizeof(aux->title), 1, file);
            fread(aux->artist, sizeof(aux->artist), 1, file);
            fread(&aux->offset, sizeof(aux->offset), 1, file);
            aux->next = NULL;
            
            if(j == 0){
                table[i] = aux;
                last = aux;
                continue;
            }
            
            last->next = aux;
            last = aux;
            
        }
        
    }
    
    fclose(file);
    return 0;
}