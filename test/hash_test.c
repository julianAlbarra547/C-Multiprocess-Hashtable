/*
 * hash_test.c
 * Tests para verificar el correcto funcionamiento de hash.c
 * Usa spotify_data_sample.csv (100 registros) para pruebas de integración
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/hash.h"
#include "../src/csv_reader.h"

#define SAMPLE_CSV  "../data/sample/spotify_data_sample.csv"
#define SAMPLE_IDX  "../data/sample/spotify_sample.idx"

/* ─── Utilidad para reportar resultados ─────────────────────────────────── */

int tests_passed = 0;
int tests_failed  = 0;

void report(const char *test_name, int passed) {
    if (passed) {
        printf("  [PASS] %s\n", test_name);
        tests_passed++;
    } else {
        printf("  [FAIL] %s\n", test_name);
        tests_failed++;
    }
}

/* ─── Helper: crea y llena una tabla con datos conocidos ────────────────── */

Hash_node **create_test_table() {
    Hash_node **table = malloc(HASH_TABLE_SIZE * sizeof(Hash_node *));
    if (table == NULL) return NULL;
    create_hash_table(table);
    return table;
}

void destroy_test_table(Hash_node **table) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Hash_node *current = table[i];
        while (current != NULL) {
            Hash_node *next = current->next;
            free(current);
            current = next;
        }
    }
    free(table);
}

/* ─── Test 1: hash() ─────────────────────────────────────────────────────── */

void test_hash() {
    printf("\n[Test 1] Funcion hash\n");

    /* El mismo titulo siempre produce el mismo cajón */
    unsigned int h1 = hash("Despacito");
    unsigned int h2 = hash("Despacito");
    report("Mismo titulo produce mismo cajon", h1 == h2);

    /* Insensible a mayusculas */
    unsigned int h3 = hash("despacito");
    unsigned int h4 = hash("DESPACITO");
    report("Insensible a mayusculas", h3 == h4);
    report("Minusculas y mayusculas dan el mismo cajon que el original", h1 == h3);

    /* El resultado siempre está dentro del rango válido */
    unsigned int h5 = hash("x");
    unsigned int h6 = hash("Una cancion con titulo muy largo que podria desbordar el hash si no se maneja bien");
    report("Cajon dentro del rango valido (titulo corto)",  h5 < HASH_TABLE_SIZE);
    report("Cajon dentro del rango valido (titulo largo)",  h6 < HASH_TABLE_SIZE);

    /* Titulos distintos generalmente producen cajones distintos */
    unsigned int h7 = hash("Halo");
    unsigned int h8 = hash("Bohemian Rhapsody");
    report("Titulos distintos producen cajones distintos", h7 != h8);
}

/* ─── Test 2: create_hash_table() ───────────────────────────────────────── */

void test_create_hash_table() {
    printf("\n[Test 2] Creacion de la tabla\n");

    Hash_node **table = malloc(HASH_TABLE_SIZE * sizeof(Hash_node *));
    int result = create_hash_table(table);

    report("create_hash_table retorna 0", result == 0);

    int all_null = 1;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (table[i] != NULL) {
            all_null = 0;
            break;
        }
    }
    report("Todos los cajones inicializados a NULL", all_null);

    free(table);
}

/* ─── Test 3: insert_node() ─────────────────────────────────────────────── */

void test_insert_node() {
    printf("\n[Test 3] Insercion de nodos\n");

    Hash_node **table = create_test_table();

    /* Insercion exitosa */
    int r1 = insert_node(table, "Despacito", "Luis Fonsi", 1000);
    report("Primera insercion retorna 0", r1 == 0);

    /* Duplicado exacto */
    int r2 = insert_node(table, "Despacito", "Luis Fonsi", 2000);
    report("Duplicado titulo+artista retorna 1", r2 == 1);

    /* Mismo titulo, artista distinto → NO es duplicado */
    int r3 = insert_node(table, "Despacito", "Otro Artista", 3000);
    report("Mismo titulo artista distinto se inserta (retorna 0)", r3 == 0);

    /* Mismo artista, titulo distinto → NO es duplicado */
    int r4 = insert_node(table, "La Bicicleta", "Luis Fonsi", 4000);
    report("Titulo distinto mismo artista se inserta (retorna 0)", r4 == 0);

    /* Insensible a mayusculas en insercion */
    int r5 = insert_node(table, "DESPACITO", "LUIS FONSI", 5000);
    report("Duplicado en mayusculas tambien se detecta (retorna 1)", r5 == 1);

    /* Insercion con titulo de artista multiple con comas */
    int r6 = insert_node(table, "Taki Taki", "DJ Snake, Selena Gomez", 6000);
    report("Insercion con artista multiple funciona", r6 == 0);

    destroy_test_table(table);
}

/* ─── Test 4: search_node() ─────────────────────────────────────────────── */

void test_search_node() {
    printf("\n[Test 4] Busqueda de nodos\n");

    Hash_node **table = create_test_table();

    insert_node(table, "Despacito",   "Luis Fonsi",  1000);
    insert_node(table, "La Bicicleta","Carlos Vives", 2000);
    insert_node(table, "Mi Gente",    "J Balvin",     3000);

    /* Busqueda exitosa */
    long o1 = search_node(table, "Despacito", "Luis Fonsi");
    report("Encuentra nodo existente", o1 == 1000);

    long o2 = search_node(table, "La Bicicleta", "Carlos Vives");
    report("Encuentra segundo nodo existente", o2 == 2000);

    /* Busqueda insensible a mayusculas */
    long o3 = search_node(table, "DESPACITO", "LUIS FONSI");
    report("Busqueda insensible a mayusculas", o3 == 1000);

    long o4 = search_node(table, "despacito", "luis fonsi");
    report("Busqueda en minusculas funciona", o4 == 1000);

    /* Busqueda de nodo inexistente */
    long o5 = search_node(table, "Bohemian Rhapsody", "Queen");
    report("Retorna -1 para nodo inexistente", o5 == -1);

    /* Titulo existe pero artista diferente */
    long o6 = search_node(table, "Despacito", "Artista Falso");
    report("Retorna -1 con artista incorrecto", o6 == -1);

    destroy_test_table(table);
}

/* ─── Test 5: build_index() ─────────────────────────────────────────────── */

void test_build_index() {
    printf("\n[Test 5] Construccion del indice desde CSV\n");

    Hash_node **table = create_test_table();

    int result = build_index(SAMPLE_CSV, table);
    report("build_index retorna 0", result == 0);

    /* Contar nodos insertados */
    int total = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Hash_node *current = table[i];
        while (current != NULL) {
            total++;
            current = current->next;
        }
    }
    report("Se insertaron nodos en la tabla", total > 0);
    report("El numero de nodos unicos es <= 100", total <= 100);
    printf("    → nodos unicos insertados: %d\n", total);

    destroy_test_table(table);
}

/* ─── Test 6: save_index() + load_index() ───────────────────────────────── */

void test_save_load_index() {
    printf("\n[Test 6] Persistencia del indice (save + load)\n");

    /* Construir tabla original */
    Hash_node **original = create_test_table();
    insert_node(original, "Despacito",    "Luis Fonsi",   1000);
    insert_node(original, "La Bicicleta", "Carlos Vives", 2000);
    insert_node(original, "Mi Gente",     "J Balvin",     3000);

    /* Guardar */
    int save_result = save_index(original, SAMPLE_IDX);
    report("save_index retorna 0", save_result == 0);

    /* Cargar en tabla nueva */
    Hash_node **loaded = create_test_table();
    int load_result = load_index(SAMPLE_IDX, loaded);
    report("load_index retorna 0", load_result == 0);

    /* Verificar que los datos sobrevivieron */
    long o1 = search_node(loaded, "Despacito", "Luis Fonsi");
    report("Nodo 1 sobrevive save+load", o1 == 1000);

    long o2 = search_node(loaded, "La Bicicleta", "Carlos Vives");
    report("Nodo 2 sobrevive save+load", o2 == 2000);

    long o3 = search_node(loaded, "Mi Gente", "J Balvin");
    report("Nodo 3 sobrevive save+load", o3 == 3000);

    /* Nodo inexistente sigue siendo inexistente tras cargar */
    long o4 = search_node(loaded, "Fantasma", "Nadie");
    report("Nodo inexistente sigue siendo -1 tras load", o4 == -1);

    destroy_test_table(original);
    destroy_test_table(loaded);

    /* Limpiar archivo de prueba */
    remove(SAMPLE_IDX);
}

/* ─── Test 7: build + save + load + search integración total ────────────── */

void test_full_integration() {
    printf("\n[Test 7] Integracion total: build → save → load → search\n");

    /* Construir desde CSV real de muestra */
    Hash_node **table = create_test_table();
    build_index(SAMPLE_CSV, table);
    save_index(table, SAMPLE_IDX);
    destroy_test_table(table);

    /* Cargar en tabla nueva y buscar */
    Hash_node **loaded = create_test_table();
    int load_result = load_index(SAMPLE_IDX, loaded);
    report("load_index sobre indice construido desde CSV retorna 0", load_result == 0);

    /* Verificar que la tabla tiene nodos */
    int total = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Hash_node *current = loaded[i];
        while (current != NULL) {
            total++;
            current = current->next;
        }
    }
    report("La tabla cargada tiene nodos", total > 0);
    printf("    → nodos tras load: %d\n", total);

    /* El offset retornado apunta a un registro real en el CSV */
    FILE *csv = open_csv(SAMPLE_CSV);
    if (csv != NULL) {
        /* Tomar el primer nodo que exista y verificar que su offset es legible */
        Hash_node *found = NULL;
        for (int i = 0; i < HASH_TABLE_SIZE && found == NULL; i++) {
            if (loaded[i] != NULL) found = loaded[i];
        }

        if (found != NULL) {
            Row *row = read_csv(csv, found->offset);
            report("El offset del indice apunta a un registro legible en el CSV", row != NULL);
            if (row != NULL) {
                printf("    → titulo en CSV: '%s'\n", row->title);
                free(row);
            }
        }
        close_csv(csv);
    }

    destroy_test_table(loaded);
    remove(SAMPLE_IDX);
}

/* ─── Main ──────────────────────────────────────────────────────────────── */

int main() {
    printf("========================================\n");
    printf("  Tests hash — modulo de indexacion\n");
    printf("========================================\n");

    test_hash();
    test_create_hash_table();
    test_insert_node();
    test_search_node();
    test_build_index();
    test_save_load_index();
    test_full_integration();

    printf("\n========================================\n");
    printf("  Resultado: %d/%d tests pasaron\n", tests_passed, tests_passed + tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
