# CSV UI Viewer

Aplicacion web estatica para explorar archivos CSV con busqueda, filtros, ordenamiento y vista de detalle.

## Cargar la interfaz desde consola

Esta es la forma recomendada para ejecutar la UI porque el proyecto es una pagina web estatica y el archivo de ejemplo se carga con `fetch()`. Por eso no conviene abrir `index.html` directamente: el navegador puede bloquear esa lectura si se usa `file://`.

### Paso 1. Abrir una terminal

Abre la aplicacion `Terminal` en macOS.

### Paso 2. Entrar a la carpeta del proyecto

Ejecuta este comando:

```bash
cd /Users/santiago/csv-ui-viewer
```

Este paso es necesario para que el servidor se inicie en la carpeta correcta y pueda encontrar:

- `index.html`
- `app.js`
- `styles.css`
- `data/spotify_sample.csv`

### Paso 3. Iniciar el servidor local

Ejecuta:

```bash
python3 -m http.server 4173
```

Elegi esta forma porque:

- `python3` ya viene instalado en muchos entornos o es facil de tener disponible
- `http.server` es una opcion simple para servir archivos estaticos
- no requiere instalar dependencias
- funciona bien para pruebas locales de este proyecto

Cuando ejecutes ese comando, la terminal quedara ocupada mostrando actividad del servidor. Eso es normal: significa que la UI ya quedo servida correctamente desde consola.

## Uso de la interfaz

Una vez cargada la UI:

1. Haz clic en `Cargar datos de ejemplo` para abrir el CSV incluido.
2. O usa `Subir CSV` para cargar tu propio archivo.
3. Usa `Buscar`, `Columna para filtrar`, `Valor del filtro` y `Ordenar por`.
4. Haz clic en una fila para ver sus detalles.

## Detener el servidor

En la misma terminal donde ejecutaste el servidor, presiona:

```bash
Ctrl + C
```

## Archivos principales

- `index.html`: estructura de la interfaz
- `styles.css`: estilos visuales
- `app.js`: logica de carga, filtro, ordenamiento y detalle
- `data/spotify_sample.csv`: archivo de ejemplo

## Nota

No abras `index.html` directamente con `file://`. Este proyecto funciona mejor cuando se sirve desde consola con un servidor local, porque asi el navegador puede cargar correctamente los archivos asociados del proyecto.
