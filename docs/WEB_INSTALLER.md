# Instalador web KI Room

La web en `web/` usa los cuatro paisajes existentes y ESP Web Tools.
Los botones de instalación permanecen ocultos si faltan el manifiesto o el
binario. La selección de personaje es una previsualización: ambos firmwares
incluyen los cuatro personajes, que se eligen desde el táctil.

## Compilación y publicación

El workflow `.github/workflows/ki-room.yml` compila los entornos
`ESP32-2432S028R-Room` y `ESP32_2432S028_2USB-Room`, y sus variantes
`-Static` sin animación. Después genera un instalador
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
bash tools/compile_room.sh
python3 tools/package_web.py --output _site --version ki-local
python3 -m http.server 8765 --directory _site
```

Referencias: https://esphome.github.io/esp-web-tools/ y
https://github.com/actions/deploy-pages.

## Portada animada

`web/index.html` muestra la demo interactiva con reloj y estadísticas de ejemplo,
selector de personajes, pausa, luz suave y comparación con la imagen original.
El instalador se conserva en `web/instalar.html`, enlazado desde la portada.
El firmware adapta el movimiento por filas RGB565: respiración, golpes, ki,
chispas, agua y pétalos, con protección de los textos y límite de 8 FPS.
Las pruebas de rendimiento y apariencia en placa siguen pendientes.

## Opción sin animación

La versión animada se selecciona por defecto. En el instalador, elige tu placa
y «Sin animación» para instalar o descargar el `.bin` estático, completo o de
actualización. Incluye las mismas imágenes, pantallas, reloj y datos de minería.
Los efectos se excluyen durante la compilación con `CYD_ROOM_STATIC=1`:
no hay repintados decorativos, pero se mantienen las actualizaciones de datos
y la caducidad de las notificaciones de shares. No se ha medido la diferencia
de hashrate en hardware.

`python3 tools/check_room.py` comprueba ambas variantes con ASan/UBSan;
la estática debe completar 32 ciclos sin repintados en cada escena y pantalla.
