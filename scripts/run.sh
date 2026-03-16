#!/bin/bash

# ir al directorio raíz del proyecto
cd "$(dirname "$0")/.."

FIFO1="/tmp/csv_ui_to_hash"
FIFO2="/tmp/csv_hash_to_ui"

echo "Creando FIFOs..."
[ -p $FIFO1 ] || mkfifo $FIFO1
[ -p $FIFO2 ] || mkfifo $FIFO2

echo "Compilando..."

gcc src/hash_server.c src/hash.c src/csv_reader.c -o hash_server
gcc UI/csv_ui_viewer.c -o ui

echo "Iniciando servidor..."
./hash_server &

sleep 1

echo "Iniciando UI..."
./ui