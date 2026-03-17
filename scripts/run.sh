#!/bin/bash
set -e
 
# Navega a la raíz del proyecto (un nivel arriba de scripts/)
cd "$(dirname "$0")/.."
 
CSV="${1:-data/sample/spotify_data_sample.csv}"
IDX="spotify_idx.bin"
ENTRIES="spotify_entries.bin"
 
echo "=== Compilando... ==="
make
 
# Solo indexa si no existen los .bin o si el CSV es más nuevo que el índice
if [ ! -f "$IDX" ] || [ ! -f "$ENTRIES" ] || [ "$CSV" -nt "$IDX" ]; then
    echo ""
    echo "=== Generando índice (esto tarda un momento)... ==="
    ./build_index "$CSV"
else
    echo "=== Índice ya existe, saltando build_index ==="
fi
 
echo ""
echo "=== Iniciando servidor en background... ==="
./hash_server &
SERVER_PID=$!
echo "Servidor PID: $SERVER_PID"
 
# Espera a que los FIFOs estén listos
sleep 0.5
 
echo ""
echo "=== Iniciando cliente... ==="
./csv_ui_viewer
 
# Al salir del cliente, baja el servidor
echo ""
echo "=== Cerrando servidor (PID $SERVER_PID)... ==="
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
 
echo "=== Listo ==="