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
- `bash tools/compile_room.sh`: intentado para ambos entornos; se detiene en
  la descarga de pip por `Temporary failure in name resolution`. No llegó al
  compilador ESP32. No hay binarios ni tamaño final de flash/RAM verificados.
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

El shell sigue sin resolver GitHub y no dispone de PlatformIO. Antes de
flashear, compilar los dos entornos con:

```bash
bash tools/compile_room.sh
```

No se han validado binarios ni hardware. La rama conserva las funciones y la
licencia originales. El remoto `origin` local apunta al original; para futuras
subidas usar el remoto `fork`, dirigido a NerdMiner-Anime.
