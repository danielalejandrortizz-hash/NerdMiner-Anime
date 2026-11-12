# Instalador web KI Room

La web en `web/` usa los cuatro paisajes existentes y ESP Web Tools.
Los botones de instalación permanecen ocultos si faltan el manifiesto o el
binario. La selección de personaje es una previsualización: ambos firmwares
incluyen los cuatro personajes, que se eligen desde el táctil.

## Compilación y publicación

El workflow `.github/workflows/ki-room.yml` compila los entornos
`ESP32-2432S028R-Room` y `ESP32_2432S028_2USB-Room`. Después genera un instalador
con los binarios completos y de actualización, manifiestos y SHA-256.
El artefacto `ki-room-installer` permite descargar el paquete incluso sin Pages.
Un fallo de compilación impide publicar nuevos binarios.

Para habilitar la web por primera vez, en el repositorio abre **Settings →
Pages → Build and deployment → Source → GitHub Actions**. Si es un fork con
Actions desactivado, habilita también los workflows en la pestaña Actions.
El conector usado para publicar archivos no permite cambiar estos ajustes.
Después ejecuta el workflow desde `main` o sube un cambio a `main`.

URL prevista tras un despliegue correcto:
https://danielalejandrortizz-hash.github.io/NerdMiner-Anime/

No considerar esa URL publicada hasta comprobar el resultado del despliegue.

## Uso

Abre la web por HTTPS con Chrome o Edge en un ordenador. Conecta la CYD con
un cable USB de datos, elige uno o dos USB según tu placa y pulsa instalar.
La instalación completa incluye bootloader, particiones y firmware. Puede
borrar ajustes. La actualización solo es apropiada si ya tienes KI Room con
las mismas particiones; desmarca el borrado en el diálogo para conservar datos.
Después configura Wi-Fi, dirección pública de wallet y pool en el portal del
dispositivo. No se ha probado físicamente el firmware en una placa.

## Vista previa local

Para ver el diseño sin compilar, copia los paisajes a `web/assets/` y sirve
`web/` con un servidor estático. Esos archivos ya están en `artwork/ki-room/`.
No se ofrecen instalaciones sin los binarios. Para el paquete completo:

```bash
python3 -m pip install platformio==6.1.18 esptool==4.8.1
pio run -e ESP32-2432S028R-Room -e ESP32_2432S028_2USB-Room
python3 tools/package_web.py --output _site --version ki-local
python3 -m http.server 8765 --directory _site
```

Referencias: https://esphome.github.io/esp-web-tools/ y
https://github.com/actions/deploy-pages.
