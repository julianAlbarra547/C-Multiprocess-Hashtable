# Diseño de la tabla Hash

## Objetivo

Construir un índice en memoria que permita buscar registros del dataset de Spotify por título y artista en menos de 2 segundos, sin cargar el CSV completo en RAM.

## Estructura del nodo

Cada entrada del índice almacena únicamente los campos necesarios para buscar y localizar un registro:

```c
typedef struct hash_node {
    char title[512];         // criterio de búsqueda primario
    char artist[2048];       // criterio de búsqueda secundario
    long offset_csv;         // posición en bytes del registro en el CSV
    struct hash_node *next;  // siguiente nodo en la lista enlazada
} Hash_node;
```

El campo `offset_csv` permite saltar directamente al registro en el CSV con `fseek()` sin necesidad de cargarlo completo en memoria. El resto de los campos del registro (álbum, duración, streams, etc.) se leen del CSV únicamente cuando se encuentra una coincidencia.

## Dimensionamiento de la tabla

Se eligieron 9.973 cajones (número primo de 4 cifras) por recomendación del enunciado. Los números primos reducen colisiones al tener menos factores comunes con los valores hasheados.

## Función hash — algoritmo djb2

```c
unsigned int hash(char *title) {
    unsigned int h = 5381;
    for (int i = 0; title[i] != '\0'; i++) {
        h = h * 33 + title[i];
    }
    return h % HASH_TABLE_SIZE;
}
```

Se utiliza el algoritmo djb2 por las siguientes razones:

- Considera el orden de los caracteres, evitando que anagramas colisionen.
- El valor inicial 5381 y el multiplicador 33 fueron determinados empíricamente para producir distribuciones uniformes sobre texto

Antes de hashear, el título se normaliza a minúsculas con `normalize_string()`para garantizar búsquedas insensibles a mayúsculas.

## Manejo de colisiones — encadenamiento

Cuando dos títulos distintos producen el mismo cajón, los nodos se encadenan en una lista enlazada. La inserción se realiza al final de la lista para mantener el orden de aparición en el CSV.

## Persistencia del índice

Construir el índice leyendo 26 millones de filas tarda varios minutos. Para evitar reconstruirlo en cada ejecución, el índice se serializa a un archivo binario `spotify.idx` con la siguiente estructura:

```
[HASH_TABLE_SIZE: int]
  [num_nodos_cajon_0: int]
    [title: 512 bytes][artist: 2048 bytes][offset: 8 bytes]
    ...
  [num_nodos_cajon_1: int]
    ...
  ...
```

Los punteros `next` no se serializan porque son direcciones de RAM inválidas en disco. Al cargar el índice, se reconstruyen los enlaces leyendo cada cajón secuencialmente.
