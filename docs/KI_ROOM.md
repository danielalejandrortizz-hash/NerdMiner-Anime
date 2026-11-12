# KI Room — integración con NerdMiner Modded

Interfaz decorativa para las CYD de 2,8 pulgadas (320×240), añadida sobre
`522764d` en la rama `feature/ki-room-theme`. El motor de minería, Stratum,
wallet/pool/backup, API, MQTT, Wi-Fi y los ajustes web conservan su código.

## Qué incluye

- Cuatro paisajes: Goku, Super Saiyan, Pikachu y Charmander.
- Pantalla principal: reloj, fecha, hashrate en KH/s, mejor dificultad local,
  workers del pool, shares aceptadas/rechazadas, conexión y tiempo encendido.
- Datos transparentes en la parte izquierda, sin tarjetas sobre los personajes.
- Reloj con dígitos centrados en (160,120) y fecha situada en el cielo.
- Mercado Pulse original, con precio, bloque, comisiones, red y halving.
- Selector táctil de escenas y opción para volver a Pulse en el mismo firmware.
- Tema y última pantalla guardados en el espacio NVS existente `cyd_ui`.
- Se conservan rotación, inversión, brillo, timeout y atenuación nocturna del upstream.

Los paisajes son estáticos. Hay una luz ambiental sutil y una notificación al
confirmar una share de dificultad >=1. No hay animación articulada del pico.

## Controles en la placa

- Mitad derecha de la pantalla: siguiente página.
- Mitad izquierda: página anterior.
- Esquina superior derecha: apagar/encender pantalla (gesto del upstream).
- Con pantalla apagada, el primer toque solo la despierta.
- Zona superior izquierda en paisaje/reloj: siguiente personaje.
- En Mercado o Pulse, arriba a la izquierda sigue cambiando el acento de Pulse.
- Las páginas ciclan MINA → MERCADO → RELOJ → TEMAS.
- En TEMAS, tocar una tarjeta elige el personaje y vuelve a MINA.
- `VOLVER A PULSE / ORIGINAL` restaura el aspecto Pulse. TEMAS sigue disponible
  al ciclar las páginas para poder regresar a los paisajes.

Las coordenadas de tarjetas y zonas de toque están definidas en el driver.
Se mantienen la calibración y la interpretación de rotación originales.

## De dónde salen los datos

| Información | Fuente existente |
| --- | --- |
| KH/s y mejor dificultad local | `getMiningData()` |
| Aceptadas/rechazadas | `getMinerStatus()`, respuestas del pool desde el arranque |
| Workers | `pData.workersCount` tras `getPoolData()` |
| Fecha y hora | Reloj/NTP y zona horaria configurados en el dispositivo |
| Uptime | `mining_data.timeMining` |
| Estado de conexión | Wi-Fi, suscripción y autorización de Stratum |

El valor de workers es el total del pool para la dirección, no los núcleos del
ESP32. Sin una respuesta válida se muestra `--`; el último dato válido sigue la
cadencia original del pool, que puede tener hasta 15 minutos. No se añaden
peticiones de red. Desconectado, KH/s y workers se muestran como desconocidos.

La etiqueta original SHARES de Pulse conserva su significado (shares >=1,
incluidas las estadísticas persistidas). Las cifras OK/KO del paisaje son
todas las respuestas del pool desde el arranque; por eso no tienen por qué
coincidir con la cifra de Pulse. Los contadores largos se abrevian visualmente.

## Compilación

Con PlatformIO instalado:

```bash
pio run -e ESP32-2432S028R-Room
pio run -e ESP32_2432S028_2USB-Room
```

O, desde esta Debian:

```bash
bash ~/compilar
```

El atajo llama a `tools/compile_room.sh`, que instala las herramientas en
`.pio/build-tools` si faltan y compila ambos entornos. Requiere internet para
descargar pip, PlatformIO, el SDK y las dependencias. No requiere root, no
modifica paquetes del sistema ni flashea dispositivos.

Los binarios se generan en `.pio/build/<entorno>/firmware.bin`. El post-build
del upstream también prepara los archivos `factory` y `firmware` en
`firmware/<version>/`. El registro de compilación queda en `.pio/room-build.log`.
La variante comercial exacta del dispositivo todavía no está confirmada.

Los entornos base y Classic no activan este tema ni incluyen sus bitmaps.
Los nuevos entornos heredan los mismos pines, partición y flags de minería.

## Gráficos y memoria

Las imágenes originales están en `artwork/ki-room/`. Se han generado para este
concepto; los personajes pertenecen a sus titulares y no forman parte de una
afiliación oficial con las franquicias.

```bash
python3 tools/build_room_assets.py
```

La herramienta convierte PNG RGB/RGBA opacos a 320×240 RGB565, con promedio
por área. Usa solo la biblioteca estándar de Python. Cada imagen ocupa
153.600 bytes de flash; las cuatro ocupan 614.400 bytes antes del resto del
firmware. Las tablas se incluyen como `const PROGMEM` únicamente en Room.

El fondo se transmite por filas (640 bytes temporales) al cambiar de pantalla
o escena. Los textos se componen restaurando el fragmento original de imagen
en un sprite pequeño, sin rectángulos de fondo. Solo se actualizan cuando
cambian. El widget más grande usado por Room requiere 9.976 bytes de píxeles;
esto no incluye la sobrecarga de TFT_eSPI/heap ni las otras tareas.

## Comprobaciones realizadas y pendientes

```bash
python3 tools/check_room.py
```

Compila el renderer Room y las cuatro tablas reales con g++ y un display de
escritorio, ejecuta ASan/UBSan (sin LeakSanitizer) y comprueba:

- Límites de las transferencias y consumo máximo de píxeles del widget.
- Que las estadísticas no modifiquen el área derecha de los personajes.
- Que datos sin cambios no provoquen nuevas transferencias de widgets.
- Cambio entre cuatro escenas, reloj y selector; estado sin red.
- Que no se celebre la primera lectura histórica y que expire la notificación.

Genera PNG de revisión en `artwork/ki-room/checks/`. Estos usan los gráficos y
fuentes embebidos con un adaptador de escritorio; no son capturas de una placa.

**Compilación verificada en GitHub Actions:** ambos entornos Room compilan.
Usan 58.536 bytes de RAM estática y aproximadamente 2,47 MB de flash
(78,4 % de la partición de aplicación). Esto no mide el heap durante el uso.
**Pendiente:** pruebas de touch/rotación/inversión y comparación de hashrate
en hardware. Consulta [el instalador web](WEB_INSTALLER.md) para instalar.
La rama de entrega es `feature/ki-room-theme` en el fork
https://github.com/danielalejandrortizz-hash/NerdMiner-Anime.
No se ha flasheado ningún dispositivo.
