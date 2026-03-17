"""
explore_dataset.py
Muestra títulos y artistas reales del dataset para usar en tests agresivos.
Uso: python3 explore_dataset.py
"""

import csv
import random

CSV_PATH = "../data/raw/spotify_data.csv"

def load_unique_entries(path, limit=500000):
    """Carga combinaciones únicas título+artista hasta el límite dado."""
    entries = {}
    count = 0

    print(f"Leyendo dataset (primeras {limit:,} filas)...")

    with open(path, encoding="utf-8") as f:
        reader = csv.reader(f)
        next(reader)  # saltar header

        for row in reader:
            if len(row) < 5:
                continue

            title  = row[1].strip()
            artist = row[4].strip()
            key    = (title.lower(), artist.lower())

            if key not in entries:
                entries[key] = (title, artist)

            count += 1
            if count >= limit:
                break

            if count % 100000 == 0:
                print(f"  Procesadas {count:,} filas, {len(entries):,} únicas...")

    return list(entries.values())


def show_samples(entries):
    """Muestra muestras representativas para copiar en el test."""

    print(f"\n{'='*60}")
    print(f"  Total entradas únicas encontradas: {len(entries):,}")
    print(f"{'='*60}")

    # Muestra aleatoria de 20 entradas
    sample = random.sample(entries, min(20, len(entries)))

    print("\n── 20 entradas aleatorias para el test ──")
    for i, (title, artist) in enumerate(sample):
        print(f'  [{i+1:2}] titulo="{title}" | artista="{artist}"')

    # Artistas con más canciones
    from collections import Counter
    artist_count = Counter(artist for _, artist in entries)
    top_artists  = artist_count.most_common(5)

    print("\n── Top 5 artistas con más títulos únicos ──")
    for artist, count in top_artists:
        print(f"  {count:4} títulos → {artist}")

    # Títulos que aparecen con múltiples artistas
    from collections import defaultdict
    title_artists = defaultdict(list)
    for title, artist in entries:
        title_artists[title.lower()].append(artist)

    multi = [(t, a) for t, a in title_artists.items() if len(a) > 1]
    multi.sort(key=lambda x: len(x[1]), reverse=True)

    print("\n── Top 5 títulos con múltiples artistas (útil para test de colisiones) ──")
    for title, artists in multi[:5]:
        print(f"  '{title}' → {len(artists)} artistas distintos")
        for a in artists[:3]:
            print(f"      · {a}")
        if len(artists) > 3:
            print(f"      · ... y {len(artists)-3} más")

    # Títulos con caracteres especiales
    special = [(t, a) for t, a in entries
               if any(c in t for c in "áéíóúñüÁÉÍÓÚÑÜ'()")]

    print(f"\n── 5 títulos con caracteres especiales (tildes, ñ, paréntesis) ──")
    for title, artist in special[:5]:
        print(f'  titulo="{title}" | artista="{artist}"')

if __name__ == "__main__":
    entries = load_unique_entries(CSV_PATH)
    show_samples(entries)
