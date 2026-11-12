# KI Room — estado de preparación del fork

Revisión local: 12 de noviembre de 2026.

## Base y cambios conservados

- Original: https://github.com/HenrysCat/NerdMiner-Modded
- Commit base: `522764dae9c579ca8a37839afc65f2a634f81fd7`.
- Rama local: `feature/ki-room-theme`.
- Cuatro escenas RGB565: Goku, Super Saiyan, Pikachu y Charmander.
- Integración en el driver CYD, reloj, estadísticas y selector táctil con opción Pulse.
- Entornos: `ESP32-2432S028R-Room` y `ESP32_2432S028_2USB-Room`.
- Se conservan README, configuración, drivers, imágenes, herramientas y licencia original.
- Sin commit local: no hay `user.name` ni `user.email` configurados en Git.
  No se ha inventado una identidad.

## Validación realizada

- `python3 tools/check_room.py`: PASS con g++, ASan y UBSan; cuatro escenas,
  reloj, menú, desconexión, caché y expiración de notificación de share.
  Mayor widget: 9.976 bytes de píxeles. LeakSanitizer está desactivado por el sandbox.
- `python3 tools/build_room_assets.py`: PASS; los cuatro `.inc` regenerados
  coinciden byte a byte con los existentes (comparación SHA-256).
- `bash -n tools/compile_room.sh` y `git diff --check`: PASS.
- Revisión de herencia de los dos entornos Room: mantienen flags, pines y
  particiones base; 2USB conserva `TFT_INVERSION_ON`.
- Motor de minería, Stratum y licencia sin cambios respecto a la base.
- Compilación real en GitHub Actions: PASS para `ESP32-2432S028R-Room` y
  `ESP32_2432S028_2USB-Room`. RAM estática: 58.536 / 327.680 bytes (17,9 %).
  Flash: 2.466.217 y 2.466.337 / 3.145.728 bytes (78,4 %).
- Instalador empaquetado con esptool: imágenes completas a offset 0 y
  actualizaciones a 0x10000, manifiestos ESP Web Tools y SHA-256.
- Ejecución verificada: https://github.com/danielalejandrortizz-hash/NerdMiner-Anime/actions/runs/43226726953.
- El shell local sigue sin resolver los hosts de dependencias.
- Sin pruebas en placa: touch, rotación, inversión, memoria y hashrate pendientes.
- Los PNG en `artwork/ki-room/checks/` son del adaptador de escritorio.
  La demo de `../NerdMiner-Anime/preview/index.html` usa cifras simuladas y
  tampoco valida el firmware.

## Publicación

Fork verificado: https://github.com/danielalejandrortizz-hash/NerdMiner-Anime

Rama de entrega: `feature/ki-room-theme`, basada en `522764d`.
El fork deriva de HenrysCat/NerdMiner-Modded. La publicación se realiza mediante
el conector GitHub, que usa la identidad autenticada para el commit; no se
configura una identidad inventada en Git local.

La rama se ha incorporado a `main`, con instalador temático en `web/` y
workflow de compilación y despliegue. Véase `docs/WEB_INSTALLER.md`.
Los binarios compilados se descargan del artefacto `ki-room-installer` de
GitHub Actions. Para recompilar localmente con acceso a internet:

```bash
bash tools/compile_room.sh
```

Se han compilado ambos firmwares y generado sus paquetes; no se ha probado hardware. La rama conserva las funciones y la
licencia originales. El remoto `origin` local apunta al original; para futuras
subidas usar el remoto `fork`, dirigido a NerdMiner-Anime.

## Animación del firmware

Los cuatro personajes se animan desde `roomTheme.inc`, usando imágenes en flash
y filas de hasta 140 píxeles, sin nuevas asignaciones de heap. Las actualizaciones
se limitan a 125 ms e incluyen movimiento local del personaje, efectos de energía
y ondas en el agua. No escriben sobre las regiones reservadas a textos.

El adaptador de escritorio comprueba los cuatro personajes en las cinco páginas,
32 fases de animación, conservación de widgets, limitación de frecuencia, y ausencia
de repintado en TEMAS y Pulse, con ASan/UBSan. No se ha probado una placa física;
fluidez, consumo real de memoria y efecto sobre el hashrate siguen sin medir.
