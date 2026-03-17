## Test Folder

En esta carpeta se encuentran tests realizados a las principales funciones
del programa. Gran parte de los tests han sido realizados con apoyo de IA,
pero comprendidos y validados manualmente con datos reales.

## Archivos

* **csv_reader_test.c** → Prueba las funciones de `csv_reader.c` usando el
  dataset de muestra `../data/sample/spotify_data_sample.csv`. Cubre apertura
  de archivo, lectura del primer registro, acceso aleatorio por offset, campos
  entre comillas, tipos numéricos y lectura de los 100 registros completos.

* **hash_stress_test.c** → Test agresivo usando el dataset real de 4GB.
  Construye o carga el índice completo, verifica que
  todas las búsquedas se ejecutan en menos de 2 segundos, comprueba
  insensibilidad a mayúsculas, manejo de títulos con múltiples artistas y
  que los offsets retornados apuntan a registros reales y legibles en el CSV.

* **test_back.c** → Backend de prueba para verificar la comunicación entre procesos via FIFOs. Simula las respuestas del servidor sin necesidad de tener el índice hash construido. Cubre búsqueda exitosa de "Solitaria" de Alkilados, respuesta NA para canciones no encontradas, e inserción de un nuevo registro con confirmación.
