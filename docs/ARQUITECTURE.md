# Arquitectura del Sistema

## Visión General

El sistema está compuesto por dos procesos independientes no emparentados que se comunican entre sí para gestionar búsquedas sobre un dataset CSV de Spotify de aproximadamente 4 GB.

```
┌─────────────────┐                    ┌─────────────────────┐
│   ui_process    │ ←── IPC (TBD) ───→ │   csv_process       │
│                 │                    │                      │
│  Interfaz de    │                    │  Búsqueda en índice  │
│  usuario        │                    │  hash + lectura CSV  │
└─────────────────┘                    └─────────────────────┘
                                                │
                                    ┌───────────┴───────────┐
                                    │                       │
                               spotify.idx            spotify_data.csv
                               (índice binario)        (dataset, 4 GB)
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

### hash

**Archivos:** `hash.h`, `hash.c`

**Responsabilidad:** Construir, persistir y consultar un índice hash sobre el campo título del dataset para localizar registros en O(1) promedio.

**Estructura principal:**

```c
typedef struct hash_node {
    char  title[512];
    char  artist[2048];
    long  offset_csv;
    struct hash_node *next;
} Hash_node;
```

**Funciones expuestas:**

| Función | Descripción |
|---|---|
| `create_hash_table(table)` | Inicializa todos los cajones a NULL |
| `hash(title)` | Calcula el cajón usando djb2 |
| `normalize_string(in, out, size)` | Convierte a minúsculas para búsqueda insensible. |
| `insert_node(table, title, artist, offset)` | Inserta si no existe la combinación título+artista. |
| `search_node(table, title, artist)` | Retorna el offset en el CSV o -1 si no existe. |
| `build_index(csv_path, table)` | Lee el CSV completo y llena la tabla en RAM |
| `save_index(table, idx_path)` | Serializa la tabla a un archivo binario `.idx`. |
| `load_index(idx_path, table)` | Deserializa el `.idx` y reconstruye la tabla en RAM. |

Para el diseño detallado del módulo hash ver [HASH_DESIGN.md](HASH_DESIGN.md).

