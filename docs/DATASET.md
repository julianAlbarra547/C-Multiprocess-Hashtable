## Dataset Overview

El dataset contiene información de canciones presentes en los charts de Spotify,
incluyendo métricas de popularidad, características de audio y metadatos del track.

El dataset incluye información como:

- nombre de la canción
- artista
- popularidad
- métricas de audio (danceability, energy, tempo, etc.)

Estos datos permiten realizar consultas sobre canciones según diferentes criterios.

## Source

El dataset fue obtenido de Kaggle:

Spotify Charts + Audio Data

https://www.kaggle.com/datasets/sunnykakar/spotify-charts-all-audio-data

Este dataset combina información de charts de Spotify con características de audio
extraídas mediante la API de Spotify.

## Dataset Structure

El dataset se encuentra en formato CSV.

Cada fila representa una canción.

Cada columna representa un atributo de la canción.

Formato general:

\# (ID) | Title | Rank | Date | Artist | URL | Region | Chart | Trend | Streams

## Fields Description

| Field | Description |
|------|-------------|
| # | Identificador del registro en el dataset - Type: Int |
| Title | Nombre de la canción - Type: Str |
| Rank | Posición de la canción en el ranking - Type: Int |
| Date | Fecha de actualización del ranking - Type: Date |
| Artist | Nombre del artista - Type: Str |
| Url | Dirección de la canción en url - Type: Str |
...

## Searchable Fields

El sistema permitirá realizar consultas usando los siguientes campos:

Primary Search Field:
- ???

Secondary Search Fields:
- ???
- ???

## Indexing Strategy

Para permitir búsquedas rápidas se implementará una tabla hash.

La clave principal será:

???

La función hash generará una posición en una estructura de índice que
apunta al offset del registro dentro del archivo CSV.

Esto permite acceder directamente al registro sin cargar el dataset completo en memoria.

## Dataset Storage

El dataset real se almacenará en:

data/raw/dataset.csv

Debido a su tamaño (>1GB), el dataset no se incluye en el repositorio.

Para pruebas se utiliza un dataset reducido:

data/sample/sample.csv

## Dataset Size

El dataset contiene aproximadamente ???  registros.

El tamaño total del archivo es aproximadamente 27 GB.
