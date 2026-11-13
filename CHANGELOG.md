# Historial de versiones

[Volver al README](README.md)

## V1.6.9 · 13 de noviembre de 2026

- Pétalos repartidos por toda la pantalla desde el primer fotograma, con
  posiciones independientes, distintas velocidades, balanceos y giros.
- Eliminadas las trayectorias regularmente espaciadas que formaban filas.
- Misma distribución en la web y el firmware. Pruebas de cobertura de los
  cuatro cuadrantes, separación vertical y equivalencia entre ambos códigos.

## V1.6.8 · 13 de noviembre de 2026

- El firmware toma la web como referencia: la demo no se ha simplificado ni
  modificado para adaptarla a la placa.
- Rehecha la composición de Paisaje, Reloj, Bitcoin, Minería y Temas con sus
  posiciones y tamaños de texto, colores, velos y paneles translúcidos.
- Fuentes sans-serif suavizadas en los tamaños CSS, guardadas en flash; día y
  fecha en español, unidades pequeñas y miniaturas con el recorte de la web.
- Animación trasladada desde `web/animation.js`: agua, movimiento de brazos,
  auras, chispas, fuego y pétalos con las mismas coordenadas, tiempos y colores.
- El fondo del selector también se anima. Se elimina la zona táctil oculta que
  activaba Pulse; una preferencia antigua de ese tema vuelve a Goku.
- Notificación de share con los textos y duración de tres segundos de la demo.
- Comparación automática entre los dos códigos: 20 muestras de los cuatro
  personajes y más de 8.000 valores de posición, color y tiempo.
- Conservadas las pruebas de navegación, protección de los textos, falta de
  memoria, repintado limpio y versiones estáticas.
- Pendiente comprobar en la placa los colores físicos, fluidez y hashrate;
  la rasterización LCD/RGB565 y la cadencia difieren del navegador.

## V1.6.7 · 13 de noviembre de 2026

- Pétalos más visibles en la demo y el firmware: 28 en vez de 20, mayor tamaño
  y tonos rosa y crema más luminosos.
- En la placa, los pétalos se actualizan cada 62 ms (hasta unas 16 veces por
  segundo), con posiciones fraccionarias y bordes suavizados; la escena conserva
  su ritmo anterior. El bucle de pantalla despierta con más frecuencia para
  permitir esa cadencia; el muestreo de datos sigue siendo de un segundo.
- Trayectorias calculadas una vez por fotograma y repintado limitado a las zonas
  que cambian, borrando también las posiciones anteriores de cada pétalo.
- En la web, los pétalos tienen balanceo, variación de tamaño y una sombra suave.
- Pruebas de movimiento, protección de los textos, borrado de rastros y límite
  de píxeles enviados por fotograma. El rendimiento real requiere prueba en placa.

## V1.6.6 · 13 de noviembre de 2026

- Historial de versiones trasladado a este archivo para acortar el README.
- El README muestra la versión actual y un enlace al historial completo.

## V1.6.5 · 13 de noviembre de 2026

- Corregida la selección del fondo al entrar en HASHRATE: la página 5 no
  estaba incluida en la ruta KI Room y preparaba los paneles negros del reloj
  Pulse. Ahora restaura el paisaje completo antes de pintar los datos.
- HASHRATE tampoco ejecuta la animación de Pulse al usar ese tema.
- La prueba del renderizador ahora ejecuta las funciones reales `paintChrome`
  y `ensureChrome` del driver, además de los widgets. Comprueba todas las páginas
  y temas y compara cada píxel del fondo con el paisaje esperado. Esta prueba
  reproduce el fallo con el código anterior.
- La verificación física en la placa sigue pendiente.

## V1.6.4 · 13 de noviembre de 2026

- Los números grandes de HASHRATE y RELOJ se componen fila a fila sobre el
  paisaje, sin crear ni liberar sprites y sin reutilizar el estado de fuente
  de otro widget. Cada actualización restaura toda su zona antes de dibujar.
- El reloj aparece desde la primera entrada y al volver a la página, con la
  hora local disponible. Ya no espera consultas HTTPS de precio o bloques.
  Hasta sincronizar la hora muestra `--:--` y un aviso.
- Nuevas pruebas de navegación repetida, cambios de cifras largas a cortas,
  primera entrada al reloj y fallos simulados de reserva de memoria.
- Las pruebas automáticas no verifican el comportamiento eléctrico de la pantalla;
  queda pendiente comprobar esta corrección en una placa física.

## V1.6.3 · 13 de noviembre de 2026

- La pantalla HASHRATE usa la media local de los últimos 10 segundos, igual que
  `/api/status`, sin depender del intervalo de refresco de la página.
- La animación respeta el título, las unidades y los textos de esa pantalla.
- Al pie de HASHRATE aparece `http://<IP-de-la-placa>/settings` para configurar
  el minero desde un navegador conectado a la misma Wi-Fi.
- Un candidato a bloque solo se cuenta cuando el pool acepta el envío.
- El tema KI Room tiene efectos decorativos y aviso de shares; todavía no incluye
  una celebración específica de bloque. La dirección BTC configurada se envía al
  pool; la placa no realiza transferencias y el pago depende del pool.

La versión del firmware y del instalador se obtiene de `src/version.h`.
El instalador añade el identificador del commit para distinguir cada compilación.

## Correcciones anteriores de la interfaz

- El hashrate ignora los repintados con intervalo cero para no contaminar la media.
- Los kH/s grandes se muestran como un número continuo, con decimal alineado.
- La comparación del mejor intento con la red muestra `< 0.0001 %` para valores
  muy pequeños y `--` cuando faltan datos; no representa progreso acumulado.
- Pétalos visibles también delante del personaje, sin pisar las estadísticas.
- El instalador sigue la paleta cálida y las tipografías de la demo principal.

Las comprobaciones automáticas no sustituyen la prueba física de la placa.
