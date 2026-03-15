/*
 * hash_stress_test.c
 * Test agresivo del módulo hash usando el dataset real de 4GB.
 * Construye el índice completo y verifica búsquedas con datos reales.
 *
 * ADVERTENCIA: La primera ejecución puede tardar varios minutos
 *              mientras construye el índice desde el CSV completo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/hash.h"
#include "../src/csv_reader.h"

#define REAL_CSV   "../data/raw/spotify_data.csv"
#define REAL_IDX   "../data/index/spotify.idx"
#define MAX_SEARCH_SECONDS 2.0

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

/* ─── Helper: mide tiempo de una búsqueda en segundos ───────────────────── */

double measure_search(Hash_node **table, char *title, char *artist) {
    clock_t start = clock();
    search_node(table, title, artist);
    clock_t end   = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

/* ─── Helper: cuenta nodos totales en la tabla ──────────────────────────── */

int count_nodes(Hash_node **table) {
    int total = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Hash_node *current = table[i];
        while (current != NULL) {
            total++;
            current = current->next;
        }
    }
    return total;
}

/* ─── Helper: verifica si el archivo existe ─────────────────────────────── */

int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) return 0;
    fclose(f);
    return 1;
}

/* ─── Test 1: Construccion o carga del indice ───────────────────────────── */

Hash_node **test_build_or_load(int *index_existed) {
    printf("\n[Test 1] Construccion o carga del indice real\n");

    Hash_node **table = malloc(HASH_TABLE_SIZE * sizeof(Hash_node *));
    if (table == NULL) {
        printf("  [FAIL] No se pudo reservar memoria para la tabla\n");
        tests_failed++;
        return NULL;
    }
    create_hash_table(table);

    int result;
    clock_t start = clock();

    if (file_exists(REAL_IDX)) {
        *index_existed = 1;
        printf("  → Indice encontrado en disco, cargando...\n");
        result = load_index(REAL_IDX, table);
        report("load_index retorna 0", result == 0);
    } else {
        *index_existed = 0;
        printf("  → Indice no encontrado, construyendo desde CSV...\n");
        printf("  → Esto puede tardar varios minutos, por favor espere...\n");
        result = build_index(REAL_CSV, table);
        report("build_index retorna 0", result == 0);

        if (result == 0) {
            printf("  → Guardando indice en disco...\n");
            int save_result = save_index(table, REAL_IDX);
            report("save_index retorna 0", save_result == 0);
        }
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("  → Tiempo de carga/construccion: %.2f segundos\n", elapsed);

    int total = count_nodes(table);
    report("La tabla tiene nodos", total > 0);
    printf("  → Nodos unicos en la tabla: %d\n", total);

    return table;
}

/* ─── Test 2: Velocidad de búsqueda < 2 segundos ────────────────────────── */

void test_search_speed(Hash_node **table) {
    printf("\n[Test 2] Velocidad de busqueda (limite: %.1f segundos)\n", MAX_SEARCH_SECONDS);

    /* Entradas reales obtenidas del script explore_dataset.py */
    struct { char *title; char *artist; } cases[] = {
        {"Dancing in the Moonlight",          "Toploader"},
        {"You've Got The Love",               "Florence + The Machine"},
        {"Walk It Talk It",                   "Migos, Drake"},
        {"Santeria",                          "Sublime"},
        {"Hypnotized",                        "Tory Lanez"},
        {"Tutu",                              "Camilo, Pedro Capó"},
        {"Chantaje (feat. Maluma)",           "Shakira"},
        {"Vente Pa' Ca (feat. Maluma)",       "Ricky Martin"},
        {"Reggaetón Lento (Bailemos)",        "CNCO"},
        {"Otra vez (feat. J Balvin)",         "Zion & Lennox"},
    };

    int n = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < n; i++) {
        double elapsed = measure_search(table, cases[i].title, cases[i].artist);
        char test_name[256];
        snprintf(test_name, sizeof(test_name),
                 "Busqueda '%s' < 2s (%.4fs)", cases[i].title, elapsed);
        report(test_name, elapsed < MAX_SEARCH_SECONDS);
    }
}

/* ─── Test 3: Búsquedas exitosas con datos reales ───────────────────────── */

void test_real_searches(Hash_node **table) {
    printf("\n[Test 3] Busquedas exitosas con datos reales\n");

    struct { char *title; char *artist; } cases[] = {
        {"Dancing in the Moonlight",    "Toploader"},
        {"You've Got The Love",         "Florence + The Machine"},
        {"Walk It Talk It",             "Migos, Drake"},
        {"Santeria",                    "Sublime"},
        {"Tutu",                        "Camilo, Pedro Capó"},
        {"Chantaje (feat. Maluma)",     "Shakira"},
        {"Reggaetón Lento (Bailemos)",  "CNCO"},
    };

    int n = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < n; i++) {
        long offset = search_node(table, cases[i].title, cases[i].artist);
        char test_name[256];
        snprintf(test_name, sizeof(test_name),
                 "Encuentra '%s' - '%s'", cases[i].title, cases[i].artist);
        report(test_name, offset != -1);
        if (offset != -1) {
            printf("    → offset: %ld\n", offset);
        }
    }
}

/* ─── Test 4: Búsquedas insensibles a mayúsculas ────────────────────────── */

void test_case_insensitive(Hash_node **table) {
    printf("\n[Test 4] Busqueda insensible a mayusculas\n");

    long o1 = search_node(table, "Dancing in the Moonlight", "Toploader");
    long o2 = search_node(table, "DANCING IN THE MOONLIGHT", "TOPLOADER");
    long o3 = search_node(table, "dancing in the moonlight", "toploader");

    report("Original encuentra el nodo",     o1 != -1);
    report("Mayusculas dan el mismo offset",  o1 == o2);
    report("Minusculas dan el mismo offset",  o1 == o3);
}

/* ─── Test 5: Títulos con múltiples artistas (colisiones reales) ─────────── */

void test_multiple_artists(Hash_node **table) {
    printf("\n[Test 5] Titulo con multiples artistas distintos\n");

    /* 'alone' tiene 10 artistas distintos en el dataset */
    struct { char *artist; } artistas[] = {
        {"Alan Walker"},
        {"Marshmello"},
        {"yahyel"},
    };

    int n = sizeof(artistas) / sizeof(artistas[0]);
    long offsets[3];

    for (int i = 0; i < n; i++) {
        offsets[i] = search_node(table, "alone", artistas[i].artist);
        char test_name[256];
        snprintf(test_name, sizeof(test_name),
                 "Encuentra 'alone' de '%s'", artistas[i].artist);
        report(test_name, offsets[i] != -1);
    }

    /* Los offsets deben ser distintos entre sí */
    report("Artistas distintos tienen offsets distintos",
           offsets[0] != offsets[1] && offsets[1] != offsets[2]);
}

/* ─── Test 6: Búsquedas que deben retornar -1 ───────────────────────────── */

void test_not_found(Hash_node **table) {
    printf("\n[Test 6] Busquedas que deben retornar -1\n");

    long o1 = search_node(table, "Esta Cancion No Existe", "Artista Fantasma");
    report("Cancion inexistente retorna -1", o1 == -1);

    /* Titulo real pero artista incorrecto */
    long o2 = search_node(table, "Santeria", "Artista Incorrecto");
    report("Titulo real con artista incorrecto retorna -1", o2 == -1);

    /* Artista real pero titulo incorrecto */
    long o3 = search_node(table, "Titulo Inventado", "Shakira");
    report("Artista real con titulo incorrecto retorna -1", o3 == -1);
}

/* ─── Test 7: El offset apunta a un registro legible en el CSV ───────────── */

void test_offset_integrity(Hash_node **table) {
    printf("\n[Test 7] Integridad de offsets en el CSV real\n");

    FILE *csv = open_csv(REAL_CSV);
    if (csv == NULL) {
        printf("  [SKIP] No se pudo abrir el CSV real\n");
        return;
    }

    struct { char *title; char *artist; } cases[] = {
        {"Chantaje (feat. Maluma)", "Shakira"},
        {"Santeria",                "Sublime"},
        {"Tutu",                    "Camilo, Pedro Capó"},
    };

    int n = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < n; i++) {
        long offset = search_node(table, cases[i].title, cases[i].artist);

        if (offset == -1) {
            printf("  [SKIP] No se encontro '%s' en el indice\n", cases[i].title);
            continue;
        }

        Row *row = read_csv(csv, offset);
        char test_name[256];
        snprintf(test_name, sizeof(test_name),
                 "Offset de '%s' apunta a registro legible", cases[i].title);
        report(test_name, row != NULL);

        if (row != NULL) {
            printf("    → CSV dice: title='%s' artist='%s'\n",
                   row->title, row->artist);
            free(row);
        }
    }

    close_csv(csv);
}

/* ─── Main ──────────────────────────────────────────────────────────────── */

int main() {
    printf("========================================\n");
    printf("  Hash Stress Test — Dataset Real 4GB\n");
    printf("========================================\n");

    int index_existed = 0;
    Hash_node **table = test_build_or_load(&index_existed);

    if (table == NULL) {
        printf("\nNo se pudo inicializar la tabla. Abortando tests.\n");
        return 1;
    }

    test_search_speed(table);
    test_real_searches(table);
    test_case_insensitive(table);
    test_multiple_artists(table);
    test_not_found(table);
    test_offset_integrity(table);

    /* Liberar tabla */
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Hash_node *current = table[i];
        while (current != NULL) {
            Hash_node *next = current->next;
            free(current);
            current = next;
        }
    }
    free(table);

    printf("\n========================================\n");
    printf("  Resultado: %d/%d tests pasaron\n", tests_passed, tests_passed + tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
