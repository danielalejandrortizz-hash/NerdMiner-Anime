
## Pantallas Bitcoin y detalle de minería

Bitcoin usa ahora la ilustración seleccionada, con precio, último bloque,
comisión a 30 minutos, dificultad de red y bloques hasta el halving. El reloj
incluye «diseñado por Dani con ayuda de codex».

Después de Temas, la navegación lateral incluye una quinta pantalla con el
mejor intento local, dificultad de red, comparación en porcentaje, shares
aceptadas/rechazadas de dificultad >= 1 y hashrate. El porcentaje es
`getBestDiff() / dificultad de red * 100`: compara dificultades, no expresa
progreso acumulado ni probabilidad del siguiente hash. La dificultad de red
procede del valor abreviado del monitor, por lo que la comparación es aproximada.
Sin objetivo o sin mejor intento se muestra `--`. Las shares de este detalle
son las de dificultad >= 1, no todas las shares de baja dificultad del pool.

Validación: `python3 tools/check_room.py` genera capturas de las cuatro escenas,
mercado, detalle y reloj, comprueba caché, límites de dibujo y widgets de máximo
10.000 bytes con ASan/UBSan. No sustituye la compilación y prueba del ESP32.
