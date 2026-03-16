#include <stdio.h>
#include <stdlib.h>
 
#include "hash.h"
 
/*
 * build_index — uso:
 *   ./build_index <archivo.csv>
 *
 * Genera dos archivos binarios en el directorio actual:
 *   spotify_idx.bin      tabla hash (offsets por bucket)
 *   spotify_entries.bin  nodos de la lista encadenada
 *
 * Solo necesitas correr esto una vez, o cada vez que
 * reemplaces el CSV desde cero.
 */
 
int main(int argc, char *argv[]) {
 
    const char *csv_path = "data/sample/spotify_data_sample.csv";
 
    if (argc >= 2) {
        csv_path = argv[1];
    }
 
    printf("Indexando: %s\n", csv_path);
    printf("Tabla:     %s\n", TABLE_IDX);
    printf("Entradas:  %s\n", ENTRIES_BIN);
    printf("Esto puede tardar unos segundos...\n\n");
 
    int result = build_index(csv_path, TABLE_IDX, ENTRIES_BIN);
 
    if (result != 0) {
        fprintf(stderr, "Error al construir el índice.\n");
        return 1;
    }
 
    printf("\nÍndice construido exitosamente.\n");
    printf("Ya puedes arrancar hash_server.\n");
 
    return 0;
}