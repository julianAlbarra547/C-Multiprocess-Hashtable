/*
 * csv_reader_test.c
 * Tests para verificar el correcto funcionamiento de csv_reader
 * Dataset: spotify_data_sample.csv (100 registros)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/csv_reader.h"

#define SAMPLE_PATH "../data/sample/spotify_data_sample.csv"
#define TOTAL_TESTS 6

/* ─── Utilidad para reportar resultados ─────────────────────────────────── */

int tests_passed = 0;
int tests_failed = 0;

void report(const char *test_name, int passed) {
    if (passed) {
        printf("  [PASS] %s\n", test_name);
        tests_passed++;
    } else {
        printf("  [FAIL] %s\n", test_name);
        tests_failed++;
    }
}

/* ─── Test 1: Apertura del archivo ──────────────────────────────────────── */

void test_open_csv() {
    printf("\n[Test 1] Apertura del archivo\n");

    FILE *csv = open_csv(SAMPLE_PATH);
    report("El archivo se abre sin errores", csv != NULL);

    if (csv != NULL) {
        fclose(csv);
    }

    FILE *bad = open_csv("archivo_que_no_existe.csv");
    report("Retorna NULL con archivo inexistente", bad == NULL);
}

/* ─── Test 2: Lectura del primer registro (offset conocido) ─────────────── */

void test_first_row() {
    printf("\n[Test 2] Lectura del primer registro\n");

    FILE *csv = open_csv(SAMPLE_PATH);
    if (csv == NULL) { printf("  [SKIP] No se pudo abrir el archivo\n"); return; }

    /* Saltar header y guardar offset del primer registro */
    skip_header(csv);
    long first_offset = ftell(csv);

    Row *row = read_csv(csv, first_offset);

    report("Row no es NULL",             row != NULL);
    report("ID es 0",                    row != NULL && row->id == 0);
    report("Title no esta vacio",        row != NULL && strlen(row->title) > 0);
    report("Artist no esta vacio",       row != NULL && strlen(row->artist) > 0);
    report("Duration es mayor que cero", row != NULL && row->duration > 0);

    if (row != NULL) {
        printf("    → id=%d title='%s' artist='%s' duration=%.0f\n",
               row->id, row->title, row->artist, row->duration);
        free(row);
    }

    fclose(csv);
}

/* ─── Test 3: Acceso aleatorio por offset ───────────────────────────────── */

void test_random_access() {
    printf("\n[Test 3] Acceso aleatorio por offset\n");

    FILE *csv = open_csv(SAMPLE_PATH);
    if (csv == NULL) { printf("  [SKIP] No se pudo abrir el archivo\n"); return; }

    skip_header(csv);

    /* Guardar offsets de los primeros 10 registros */
    long offsets[10];
    Row *tmp;

    for (int i = 0; i < 10; i++) {
        offsets[i] = ftell(csv);
        tmp = read_csv(csv, offsets[i]);
        free(tmp);
    }

    /* Ahora saltar directo al registro 5 y verificar que su id es 5 */
    Row *row5 = read_csv(csv, offsets[5]);

    report("Acceso directo al registro 5 funciona", row5 != NULL);
    report("El id del registro 5 es correcto",      row5 != NULL && row5->id == 5);

    if (row5 != NULL) {
        printf("    → id=%d title='%s'\n", row5->id, row5->title);
        free(row5);
    }

    /* Verificar que leer registro 0 luego del 5 sigue funcionando */
    Row *row0 = read_csv(csv, offsets[0]);
    report("Lectura de registro 0 despues de acceso aleatorio", row0 != NULL && row0->id == 0);

    if (row0 != NULL) free(row0);

    fclose(csv);
}

/* ─── Test 4: Registro con artista entre comillas (multiples artistas) ─── */

void test_quoted_artist() {
    printf("\n[Test 4] Registro con artista entre comillas\n");

    FILE *csv = open_csv(SAMPLE_PATH);
    if (csv == NULL) { printf("  [SKIP] No se pudo abrir el archivo\n"); return; }

    skip_header(csv);

    /*
     * Los registros con artistas entre comillas empiezan en el indice 75.
     * Recorremos hasta llegar a uno que tenga coma en el artista.
     */
    long offset;
    Row *row = NULL;
    Row *found = NULL;

    for (int i = 0; i < 100; i++) {
        offset = ftell(csv);
        row = read_csv(csv, offset);
        if (row == NULL) break;

        if (strchr(row->artist, ',') != NULL) {
            found = row;
            break;
        }
        free(row);
    }

    report("Se encontro un registro con multiples artistas", found != NULL);

    if (found != NULL) {
        /* Las comillas dobles NO deben estar en el string parseado */
        report("El artista no contiene comillas dobles",
               strchr(found->artist, '"') == NULL);

        /* El artista si debe contener la coma (es parte del valor) */
        report("El artista contiene la coma del valor original",
               strchr(found->artist, ',') != NULL);

        printf("    → artist='%s'\n", found->artist);
        free(found);
    }

    fclose(csv);
}

/* ─── Test 5: Campos numericos parsean correctamente ────────────────────── */

void test_numeric_fields() {
    printf("\n[Test 5] Campos numericos\n");

    FILE *csv = open_csv(SAMPLE_PATH);
    if (csv == NULL) { printf("  [SKIP] No se pudo abrir el archivo\n"); return; }

    skip_header(csv);

    long offset = ftell(csv);
    Row *row = read_csv(csv, offset);

    report("Streams es mayor que cero",   row != NULL && row->streams > 0);
    report("Rank esta entre 1 y 200",     row != NULL && row->rank >= 1 && row->rank <= 200);
    report("Duration esta entre 60000 y 600000",
           row != NULL && row->duration >= 60000.0 && row->duration <= 600000.0);

    if (row != NULL) {
        printf("    → rank=%d streams=%lld duration=%.0f\n",
               row->rank, row->streams, row->duration);
        free(row);
    }

    fclose(csv);
}

/* ─── Test 6: Lectura de todos los registros sin crash ──────────────────── */

void test_read_all() {
    printf("\n[Test 6] Lectura de los 100 registros\n");

    FILE *csv = open_csv(SAMPLE_PATH);
    if (csv == NULL) { printf("  [SKIP] No se pudo abrir el archivo\n"); return; }

    skip_header(csv);

    int count = 0;
    int errors = 0;
    long offset;
    Row *row;

    for (int i = 0; i < 100; i++) {
        offset = ftell(csv);
        row = read_csv(csv, offset);

        if (row == NULL) {
            errors++;
        } else {
            count++;
            free(row);
        }
    }

    report("Se leyeron los 100 registros", count == 100);
    report("No hubo errores de lectura",   errors == 0);
    printf("    → leidos=%d errores=%d\n", count, errors);

    fclose(csv);
}

/* ─── Main ──────────────────────────────────────────────────────────────── */

int main() {
    printf("========================================\n");
    printf("  Tests csv_reader — spotify_data_sample\n");
    printf("========================================\n");

    test_open_csv();
    test_first_row();
    test_random_access();
    test_quoted_artist();
    test_numeric_fields();
    test_read_all();

    printf("\n========================================\n");
    printf("  Resultado: %d/%d tests pasaron\n", tests_passed, tests_passed + tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
