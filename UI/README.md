El programa consiste de un menu que actua como cliente de un sistema IPC, enviando comandos (cargar, buscar, añadir) a un servidor hash a través de tuberías con nombre (FIFOs) y mostrando sus respuestas.

### Setup: Creacion de los FIFO's con mkfifo /tmp/csv_ui_to_hash & mkfifo /tmp/csv_hash_to_ui
### Requeriments: GCC, OS compatble con POSIX.

### Proccedure:

1. Inicializar el Hash_Server y luego ejecutar en consola ./csv_ui.
2. Conectarse a los FIFO's con un path especifico o presionar Enter para usar los paths por default.
3. Buscar un registro con 'name' o 'hash_pointer'.
4. Anadir un registro con 'name, byte, int'.

### Error handling:


563
Actualmente no hay lector en el FIFO
El servidor hash no está en ejecución
Inicia el servidor hash antes de la interfaz de usuario

FIFO de escritura no existe
El archivo FIFO nunca se creó
Ejecuta mkfifo /tmp/csv_ui_to_hash

FIFO de lectura no existe
El archivo FIFO nunca se creó
Ejecuta mkfifo /tmp/csv_hash_to_ui

No hubo respuesta (tiempo de espera agotado)
El servidor no respondió en 2 segundos
Verifica que el servidor esté funcionando correctamente

El servidor hash cerró la llamada por FIFO
El servidor cerró su extremo de la tubería
Reinicia el servidor hash

FIFO no conectado
Intentaste enviar un comando antes de conectar
Usa la opción 1 primero
