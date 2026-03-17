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

#define REAL_CSV       "../data/raw/spotify_data.csv"
#define REAL_IDX       "../data/index/spotify_idx.bin"
#define REAL_ENTRIES   "../data/index/spotify_entries.bin"
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

/* ─── Helper: verifica si un archivo existe ─────────────────────────────── */

int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) return 0;
    fclose(f);
    return 1;
}

/* ─── Helper: mide tiempo de una búsqueda en segundos ───────────────────── */

double measure_search(long *table, FILE *entries, char *title, char *artist) {
    clock_t start = clock();
    search_node(table, entries, title, artist);
    clock_t end   = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

/* ─── Test 1: Construccion o carga del indice ───────────────────────────── */

long *test_build_or_load(FILE **entries_out) {
    printf("\n[Test 1] Construccion o carga del indice real\n");

    long *table = malloc(HASH_TABLE_SIZE * sizeof(long));
    if (table == NULL) {
        printf("  [FAIL] No se pudo reservar memoria para la tabla\n");
        tests_failed++;
        return NULL;
    }

    clock_t start = clock();
    int result;

    if (file_exists(REAL_IDX) && file_exists(REAL_ENTRIES)) {
        printf("  → Indice encontrado en disco, cargando...\n");
        result = load_table(REAL_IDX, table);
        report("load_table retorna 0", result == 0);
    } else {
        printf("  → Indice no encontrado, construyendo desde CSV...\n");
        printf("  → Esto puede tardar varios minutos, por favor espere...\n");
        result = build_index(REAL_CSV, REAL_IDX, REAL_ENTRIES);
        report("build_index retorna 0", result == 0);

        if (result == 0) {
            result = load_table(REAL_IDX, table);
            report("load_table tras build retorna 0", result == 0);
        }
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("  → Tiempo de carga/construccion: %.2f segundos\n", elapsed);

    int occupied = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (table[i] != -1) occupied++;
    }
    report("La tabla tiene cajones ocupados", occupied > 0);
    printf("  → Cajones ocupados: %d / %d\n", occupied, HASH_TABLE_SIZE);

    long ram_bytes = HASH_TABLE_SIZE * sizeof(long);
    printf("  → RAM usada por la tabla: %ld bytes (~%ld KB)\n",
           ram_bytes, ram_bytes / 1024);
    report("Tabla ocupa menos de 10 MB", ram_bytes < 10 * 1024 * 1024);

    *entries_out = fopen(REAL_ENTRIES, "rb");
    report("entries.bin se abre correctamente", *entries_out != NULL);

    return table;
}

/* ─── Test 2: Velocidad de búsqueda < 2 segundos ────────────────────────── */

void test_search_speed(long *table, FILE *entries) {
    printf("\n[Test 2] Velocidad de busqueda (limite: %.1f segundos)\n",
           MAX_SEARCH_SECONDS);

    struct { char *title; char *artist; } cases[] = {
        {"Dancing in the Moonlight",        "Toploader"},
        {"You've Got The Love",             "Florence + The Machine"},
        {"Walk It Talk It",                 "Migos, Drake"},
        {"Santeria",                        "Sublime"},
        {"Hypnotized",                      "Tory Lanez"},
        {"Tutu",                            "Camilo, Pedro Capó"},
        {"Chantaje (feat. Maluma)",         "Shakira"},
        {"Vente Pa' Ca (feat. Maluma)",     "Ricky Martin"},
        {"Reggaetón Lento (Bailemos)",      "CNCO"},
        {"Otra vez (feat. J Balvin)",       "Zion & Lennox"},
    };

    int n = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < n; i++) {
        double elapsed = measure_search(table, entries,
                                        cases[i].title, cases[i].artist);
        char test_name[256];
        snprintf(test_name, sizeof(test_name),
                 "Busqueda '%s' < 2s (%.4fs)", cases[i].title, elapsed);
        report(test_name, elapsed < MAX_SEARCH_SECONDS);
    }
}

/* ─── Test 3: Búsquedas exitosas con datos reales ───────────────────────── */

void test_real_searches(long *table, FILE *entries) {
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
        long offset = search_node(table, entries,
                                  cases[i].title, cases[i].artist);
        char test_name[256];
        snprintf(test_name, sizeof(test_name),
                 "Encuentra '%s' - '%s'", cases[i].title, cases[i].artist);
        report(test_name, offset != -1);
        if (offset != -1) printf("    → offset: %ld\n", offset);
    }
}

/* ─── Test 4: Búsquedas insensibles a mayúsculas ────────────────────────── */

void test_case_insensitive(long *table, FILE *entries) {
    printf("\n[Test 4] Busqueda insensible a mayusculas\n");

    long o1 = search_node(table, entries, "Dancing in the Moonlight", "Toploader");
    long o2 = search_node(table, entries, "DANCING IN THE MOONLIGHT", "TOPLOADER");
    long o3 = search_node(table, entries, "dancing in the moonlight", "toploader");

    report("Original encuentra el nodo",    o1 != -1);
    report("Mayusculas dan el mismo offset", o1 == o2);
    report("Minusculas dan el mismo offset", o1 == o3);
}

/* ─── Test 5: Títulos con múltiples artistas ────────────────────────────── */

void test_multiple_artists(long *table, FILE *entries) {
    printf("\n[Test 5] Titulo con multiples artistas distintos\n");

    long o1 = search_node(table, entries, "alone", "Alan Walker");
    long o2 = search_node(table, entries, "alone", "Marshmello");
    long o3 = search_node(table, entries, "alone", "yahyel");

    report("Encuentra 'alone' de Alan Walker", o1 != -1);
    report("Encuentra 'alone' de Marshmello",  o2 != -1);
    report("Encuentra 'alone' de yahyel",       o3 != -1);
    report("Los tres tienen offsets distintos",
           o1 != o2 && o2 != o3 && o1 != o3);
}

/* ─── Test 6: Búsquedas que deben retornar -1 ───────────────────────────── */

void test_not_found(long *table, FILE *entries) {
    printf("\n[Test 6] Busquedas que deben retornar -1\n");

    long o1 = search_node(table, entries,
                          "Esta Cancion No Existe", "Artista Fantasma");
    report("Cancion inexistente retorna -1", o1 == -1);

    long o2 = search_node(table, entries, "Santeria", "Artista Incorrecto");
    report("Titulo real con artista incorrecto retorna -1", o2 == -1);

    long o3 = search_node(table, entries, "Titulo Inventado", "Shakira");
    report("Artista real con titulo incorrecto retorna -1", o3 == -1);
}

/* ─── Test 7: El offset apunta a un registro legible en el CSV ───────────── */

void test_offset_integrity(long *table, FILE *entries) {
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
        long offset = search_node(table, entries,
                                  cases[i].title, cases[i].artist);
        if (offset == -1) {
            printf("  [SKIP] No se encontro '%s'\n", cases[i].title);
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

/* ─── Test 8: node_exists detecta duplicados correctamente ──────────────── */

void test_node_exists(long *table, FILE *entries) {
    printf("\n[Test 8] node_exists detecta duplicados correctamente\n");

    /* Cancion que sabemos que existe en el indice */
    int exists1 = node_exists(table, entries, "chantaje (feat. maluma)", "shakira");
    report("node_exists retorna 1 para cancion existente", exists1 == 1);

    /* Cancion que no existe */
    int exists2 = node_exists(table, entries, "cancion fantasma", "artista inventado");
    report("node_exists retorna 0 para cancion inexistente", exists2 == 0);

    /* Titulo real con artista incorrecto */
    int exists3 = node_exists(table, entries, "santeria", "artista incorrecto");
    report("node_exists retorna 0 con artista incorrecto", exists3 == 0);
}

/* ─── Test 9: insert_node agrega un registro nuevo correctamente ─────────── */

void test_insert_node(long *table, FILE *entries) {
    printf("\n[Test 9] insert_node agrega registro nuevo\n");

    /* Verificar que no existe antes */
    int before = node_exists(table, entries, "cancion de prueba", "artista de prueba");
    report("Cancion de prueba no existe antes de insertar", before == 0);

    /* Insertar */
    FILE *entries_rw = fopen(REAL_ENTRIES, "r+b");
    if (entries_rw == NULL) {
        printf("  [SKIP] No se pudo abrir entries en modo escritura\n");
        return;
    }

    int result = insert_node(table, entries_rw,
                             "cancion de prueba", "artista de prueba", 999999);
    report("insert_node retorna 0", result == 0);

    /* Verificar que ahora existe */
    int after = node_exists(table, entries_rw, "cancion de prueba", "artista de prueba");
    report("Cancion de prueba existe despues de insertar", after == 1);

    /* Verificar que search_node la encuentra */
    long offset = search_node(table, entries_rw, "cancion de prueba", "artista de prueba");
    report("search_node encuentra el nuevo registro", offset == 999999);

    fclose(entries_rw);
}

/* ─── Main ──────────────────────────────────────────────────────────────── */

int main() {
    printf("========================================\n");
    printf("  Hash Stress Test — Dataset Real 4GB\n");
    printf("========================================\n");

    FILE *entries = NULL;
    long *table   = test_build_or_load(&entries);

    if (table == NULL || entries == NULL) {
        printf("\nNo se pudo inicializar. Abortando tests.\n");
        if (table)   free(table);
        if (entries) fclose(entries);
        return 1;
    }

    test_search_speed(table, entries);
    test_real_searches(table, entries);
    test_case_insensitive(table, entries);
    test_multiple_artists(table, entries);
    test_not_found(table, entries);
    test_offset_integrity(table, entries);
    test_node_exists(table, entries);
    test_insert_node(table, entries);

    free(table);
    fclose(entries);

    printf("\n========================================\n");
    printf("  Resultado: %d/%d tests pasaron\n",
           tests_passed, tests_passed + tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
