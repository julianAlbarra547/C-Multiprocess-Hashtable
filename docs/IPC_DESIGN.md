# Diseño de la Comunicación entre Procesos (IPC)

## Objetivo

Permitir que dos procesos no emparentados (`ui_process` y `csv_process`) intercambien datos de forma sincrónica: la UI envía criterios de búsqueda o registros nuevos, y el servidor responde con el resultado.

## Mecanismo elegido — FIFOs nombrados

Se utilizan **tuberías nombradas (FIFOs)** creadas con `mkfifo()`. A diferencia de las tuberías anónimas, los FIFOs existen como archivos especiales en el sistema de archivos y pueden ser abiertos por procesos no emparentados usando su ruta.

```
/tmp/client_fifo   ← ui_process escribe, csv_process lee
/tmp/server_fifo   ← csv_process escribe, ui_process lee
```

Se usan dos FIFOs en direcciones opuestas para lograr comunicación bidireccional, ya que un solo FIFO solo permite flujo en una dirección.

## Diagrama de comunicación

```
ui_process                          csv_process
    │                                     │
    │── write(identify) ──────────────────►│
    │── write(query / new_row) ───────────►│
    │                                     │ procesa
    │◄── read(Row / confirm) ─────────────│
```

## Protocolo de mensajes

Cada operación comienza con un entero que identifica el tipo de solicitud, seguido de la estructura de datos correspondiente.

**Operación 1 — Búsqueda:**

```c
// UI envía:
int identify = 1;
write(fdwrite, &identify, sizeof(int));
write(fdwrite, &query,    sizeof(Query));  // título + artista

// csv_process responde:
write(fdwrite, &result, sizeof(Row));  // row completo, o id == -1 si no encontró
```

**Operación 2 — Inserción:**

```c
// UI envía:
int identify = 2;
write(fdwrite, &identify, sizeof(int));
write(fdwrite, &new_row,  sizeof(Row));  // registro completo validado

// csv_process responde:
int confirm = 1;  // 1 = éxito, 0 = error
write(fdwrite, &confirm, sizeof(int));
```

## Estructuras compartidas

Definidas en `common.h` para garantizar que ambos procesos usen exactamente el mismo layout en memoria:

```c
typedef struct {
    char title[512];
    char artist[1024];
} Query;
```

La estructura `Row` se comparte desde `csv_reader.h` y contiene todos los campos del dataset.

## Ciclo de vida de los FIFOs

```
csv_process arranca
    │
    ├── mkfifo(/tmp/client_fifo)   ← crea si no existe
    ├── mkfifo(/tmp/server_fifo)   ← crea si no existe
    ├── open(client_fifo, O_RDONLY) ← bloquea hasta que UI abra su extremo
    └── open(server_fifo, O_WRONLY) ← bloquea hasta que UI abra su extremo

ui_process arranca
    │
    ├── open(client_fifo, O_WRONLY) ← desbloquea csv_process
    └── open(server_fifo, O_RDONLY) ← desbloquea csv_process

ambos procesos quedan conectados
    │
    └── loop de mensajes hasta que UI seleccione Salir
```

## Consideraciones técnicas

**Bloqueo en apertura:** `open()` sobre un FIFO bloquea hasta que ambos extremos estén abiertos. Por eso `csv_process` debe arrancar antes que `ui_process`.

**Comunicación binaria:** Los datos se transmiten como structs binarios con `write()` y `read()`, no como texto. Esto es más eficiente pero requiere que ambos procesos compartan exactamente las mismas definiciones de struct, de ahí la importancia de `common.h`.

**Sin persistencia:** Los FIFOs son canales de comunicación en memoria. Los datos que pasan por ellos no se guardan en disco.

**Limpieza:** Al terminar, `csv_process` elimina los FIFOs con `unlink()` para no dejar archivos huérfanos en `/tmp`.
