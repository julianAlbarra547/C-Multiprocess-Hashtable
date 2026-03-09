# CSV UI Viewer

Aplicacion web estatica para explorar archivos CSV con busqueda, filtros, ordenamiento y vista de detalle.

## Ejecutar localmente

Desde este directorio:

```bash
python3 -m http.server 4173
```

Luego abre `http://127.0.0.1:4173/`.

## Desplegar en GitHub Pages

Este proyecto ya incluye el workflow de GitHub Pages en `.github/workflows/deploy-pages.yml`.

Pasos:

1. Crea un repositorio en GitHub y sube este contenido.
2. Asegurate de que la rama principal sea `main`.
3. En GitHub, ve a `Settings > Pages`.
4. En `Source`, selecciona `GitHub Actions`.
5. Haz push a `main`.
6. Espera a que termine el workflow `Deploy to GitHub Pages`.
7. GitHub publicara el sitio en una URL como:

```text
https://TU_USUARIO.github.io/TU_REPOSITORIO/
```

## Notas

- La app usa rutas relativas, asi que funciona bien en GitHub Pages dentro de un subpath del repositorio.
- El archivo de ejemplo se publica junto con la app en `data/spotify_sample.csv`.
- `.nojekyll` evita que GitHub Pages procese el sitio con Jekyll innecesariamente.
