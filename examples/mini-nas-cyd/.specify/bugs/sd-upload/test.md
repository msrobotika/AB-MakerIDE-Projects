# Verificación de la reparación de escritura SD

Fecha: 2026-10-30 (Europe/Madrid). Veredicto: **verified para el síntoma comunicado; límites adicionales detallados abajo**.

## Diagnóstico físico

En la misma CYD/tarjeta, la carga de diagnóstico reprodujo una escritura parcial a 4 MHz: 48 640 bytes de 262 161, errno 5 (EIO). El espacio consultado era suficiente. La escritura a 1 MHz y lectura posterior sí completaron ambas variantes. [Resultados del diagnóstico](diagnostic-results.json).

No se ha aislado si la limitación física viene de tarjeta, lector, señal o alimentación. Se eligió una frecuencia que superó las pruebas en esta unidad.

## Carga final y comprobación

Binario cargado en COM4, SHA-256 `670030ad7d5b5e259274d0696dad5702c5f762f7b04f3bfb0d086aad95ab3906`. Compilación completa correcta: 1073298 bytes de programa y 50388 de RAM estática. Carga con dos verificaciones de hash; sin borrado total ni volcados. STATUS confirma SD montada y reloj de 1 MHz.

Tres rondas físicas en esta carga, con bloques 1436 y 512: **seis archivos de 262 161 bytes**, todos cerrados/reabiertos y comparados byte a byte. Total **1 572 966 bytes** verificados. Se retiraron los seis archivos propios; no se leyeron ni borraron archivos personales. No se formateó.

El autor confirma «Sí, se guarda y aparece» al repetir la subida desde el móvil y comprobar el listado con esta versión.

## Observación posterior desde USB

Tras el reintento solicitado, STATUS registra 2 inicios de subida y 0 escrituras parciales; el último bloque solicitado y escrito coincide (1176 bytes), errno 0, y el espacio usado pasa de 2 228 224 a 2 326 528 bytes. Es evidencia de actividad real de subida sin el fallo de escritura observado; no sustituye la confirmación del listado ni una descarga con hash. No se consultaron nombres ni contenido de archivos.

## Criterios y límites

- AC1: error exacto comunicado y repetido por el autor; fallo de E/S reproducido también por escritura directa a 4 MHz.
- AC2: persistencia tras cierre/reapertura y bytes verificados en seis archivos de ensayo. No se ha comparado el hash de una descarga móvil en esta carga.
- AC3: se evita llamar tarjeta llena a todo error; se conserva el rechazo de archivos parciales. Las rutas de error no se confunden con éxito.
- AC4: sin cambios en sesión QR, token, nombres/no sobrescritura ni calibración. Las comprobaciones de reglas se compilan y la interfaz embebida coincide con sus fuentes.

El ensayo directo de SD no demuestra por sí solo la transferencia HTTP. No se certifican todas las tarjetas, estabilidad prolongada ni persistencia después de un corte eléctrico. Las pruebas HTTP desde PC y los escaneos QR previos tienen su evidencia histórica separada. [Resultados de esta carga](results.json), [diagnóstico](assessment.md) y [cambios](fix.md).
