# Arquitectura del Sistema

## Visión General

El sistema está compuesto por dos procesos independientes no emparentados que se comunican entre sí para gestionar búsquedas sobre un dataset CSV de Spotify de aproximadamente 4 GB.

```
┌─────────────────┐                    ┌─────────────────────┐
│   ui_process    │ ←── FIFOs (IPC) ──→│   csv_process       │
│                 │                    │                      │
│  Interfaz de    │                    │  Búsqueda en índice  │
│  usuario        │                    │  hash + lectura CSV  │
└─────────────────┘                    └─────────────────────┘
                                                │
                              ┌─────────────────┼─────────────────┐
                              │                 │                 │
                     spotify_idx.bin   spotify_entries.bin   spotify_data.csv
                     (tabla hash,      (nodos del índice,    (dataset, 4 GB,
                      80 KB en RAM)     en disco)             solo lectura)
```

## Módulos implementados

### 1. csv_reader

**Archivos:** `csv_reader.h`, `csv_reader.c`

**Responsabilidad:** Abrir el CSV, posicionarse en un registro dado su offset en bytes y parsearlo en una estructura `Row`.

**Estructura principal:**

```c
typedef struct {
    int        id;
    char       title[512];
    short      rank;
    char       date[16];
    char       artist[2048];
    char       url[64];
    long long  streams;
    char       album[512];
    double     duration;
    char       explicito[8];
} Row;
```

**Funciones expuestas:**

| Función | Descripción |
|---|---|
| `open_csv(path)` | Abre el archivo CSV y retorna un `FILE *` |
| `skip_header(file)` | Descarta la primera línea del CSV |
| `read_csv(file, offset)` | Lee y parsea el registro en la posición `offset` |
| `print_row(row)` | Imprime todos los campos de un registro |
| `close_csv(file)` | Cierra el archivo CSV |

**Decisiones de diseño:**

- El parser maneja campos entre comillas dobles para soportar valores con comas internas, como artistas colaborativos: "Bonny Cepeda, Peter Cruz, Ray Polanco".
- Los tamaños de los buffers se determinaron analizando el máximo real de cada columna con un script Python (realizado con IA) sobre el dataset completo.

### 2. hash

**Archivos:** `hash.h`, `hash.c`

**Responsabilidad:** Construir, persistir y consultar un índice hash sobre el campo título del dataset para localizar registros en O(1) promedio.

**Estructura principal:**

```c
typedef struct hash_node {
    char  title[512];
    char  artist[2048];
    long  offset_csv;
    long next_entry;
} Hash_node;
```

**Funciones expuestas:**

| Función | Descripción |
|---|---|
| `hash(title)` | Calcula el cajón usando djb2 |
| `node_exists(table, entries, normalized_title, normalized_artist)` | Verifica si la combinación título+artista ya existe, retorna 1 de ser así y 0 si no  |
| `insert_node(table, entries, normalized_title, normalized_artist, offset)` | Inserta un nodo nuevo  |
| `normalize_string(in, out, size)` | Convierte a minúsculas para búsqueda insensible |
| `create_table(table)` | Inicializa todos los cajones a -1 |
| `build_index(csv_path, idx_path, entries_path)` | Lee el CSV, deduplica por título+artista y genera `spotify_idx.bin` y `spotify_entries.bin` |
| `load_table(idx_path, table)` | Carga `spotify_idx.bin` en RAM (80 KB) |
| `search_node(table, entries, title, artist)` | Busca por título y artista recorriendo nodos en disco, retorna offset en el CSV o -1 |


Para el diseño detallado del módulo hash ver [HASH_DESIGN.md](HASH_DESIGN.md).

### 3. csv_ui_viewer

**Archivo:** `csv_ui_viewer.c`

**Responsabilidad:** Ser la interfaz de usuario por la cual se puede consultar y escribir algun registro al CSV.




