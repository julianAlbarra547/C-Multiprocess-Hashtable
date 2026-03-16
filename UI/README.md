# `csv_ui_viewer`

Interfaz de terminal para interactuar con el backend de búsqueda/hash de CSV a través de una capa IPC externa.

Este programa es intencionalmente “delgado”: recolecta input del usuario, muestra resultados y delega todo el
transporte IPC a una implementación de cliente IPC (ver `ipc_client.h`).

## Funcionalidades

- **Query**: busca por `title` y opcionalmente `artist`.
- **Append**: agrega un registro proporcionando todos los campos (ya parseados).

Los payloads del protocolo binario están definidos en `../src/ipc_protocol.h`.

## Requisitos

- Sistema operativo compatible con POSIX
- `gcc` (o compilador C compatible)

## Compilación

`csv_ui_viewer.c` depende de una implementación de cliente IPC que provea las funciones declaradas en
`ipc_client.h` (por ejemplo, un `ipc_client.c`, una librería estática, u otro objeto).

Ejemplo (compilar y enlazar con tu implementación de cliente IPC):

```sh
gcc -Wall -Wextra -std=c11 -I. csv_ui_viewer.c path/to/ipc_client.c -o csv_ui_viewer
```

## Ejecución

1. Inicia tu servicio/backend IPC (definido por tu implementación).
2. Ejecuta la UI:

```sh
./csv_ui_viewer
```

3. Elige la opción **1** para conectar.
   - Cuando te pida `Endpoint IPC`, presiona **Enter** para usar los valores por defecto del cliente IPC o
     escribe un endpoint personalizado (el formato lo define tu implementación IPC).
4. Usa **Query** (opción 2) o **Append** (opción 3).

## `ipc_client.h` (qué es y por qué existe)

`ipc_client.h` define el contrato mínimo entre la UI (`csv_ui_viewer.c`) y tu componente IPC externo.
Su objetivo es desacoplar la interfaz (menú/entrada/salida) del transporte (FIFOs, sockets, subproceso, etc.).

En concreto:

- Declara un tipo opaco `IpcClient` (handle de conexión) para ocultar detalles internos de la implementación.
- Declara 4 funciones que la UI necesita para operar:
  - `ipc_client_connect(endpoint, err, err_size)`: abre una conexión al backend IPC (mecanismo definido por tu implementación).
  - `ipc_client_query(client, &IpcQuery, &IpcQueryResp, err, err_size)`: envía un Query y retorna la respuesta parseada.
  - `ipc_client_append(client, &IpcRow, &IpcAppendResp, err, err_size)`: envía un Append y retorna el estado del servidor.
  - `ipc_client_close(client)`: cierra y libera recursos.

Gracias a este contrato, `csv_ui_viewer.c` no necesita implementar FIFOs directamente ni conocer cómo se
realiza la comunicación; solo consume una API estable.

## Contrato IPC (lo que la UI espera)

La UI usa esta API mínima:

- `ipc_client_connect(endpoint, err, err_size)`
- `ipc_client_query(client, &IpcQuery, &IpcQueryResp, err, err_size)`
- `ipc_client_append(client, &IpcRow, &IpcAppendResp, err, err_size)`
- `ipc_client_close(client)`

Si una llamada falla, la implementación debe retornar `0` y escribir un mensaje legible en `err` (best effort).

## Solución de problemas

- **“No conectado.”**: primero conecta (opción 1).
- **“Fallo al conectar …”**: backend IPC apagado, endpoint inválido o falta la implementación del cliente IPC.
- **Query devuelve “No encontrado.”**: el registro no existe (el servidor retornó `status == 1`).
- **`status` distinto de cero**: error del lado del servidor; revisa el backend/logs.
