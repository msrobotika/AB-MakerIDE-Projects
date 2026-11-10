# Verificación y convergencia · Cerca de mí

9 nov 2026. Resultado: **parcial por bloqueo de compilación y prueba física pendiente**. No se afirma convergencia física ni binario válido. Proceso Spec Kit manual, sin extensiones, commits, push, upload ni agentes adicionales. Solo se han escrito archivos bajo `examples/gasolina-ms-cyd/`.

## Evidencia ejecutada

- Lectura completa del .ino previo (1252 líneas) y de instrucciones, constitución, README, tests y plantillas oficiales locales antes del código. Respaldo y hashes en constitution.md y baseline.zip.
- `node examples/gasolina-ms-cyd/tests/verificar.mjs`: pasa. Ejecuta el JavaScript real de ajustes: listas, error de lista, carrera entre provincias, ambos modos y token nuevo entre abrir y guardar. No ejecuta WebServer/NVS del ESP32.
- `node examples/gasolina-ms-cyd/tests/cerca.mjs`: pasa. CA y 10 bloques preservados por SHA-256; invariantes; lógica del C++ extraída y traducida mecánicamente a JS: filtros AP (incluidos local/multicast/nulo/_nomap/hotspot), 15 más fuertes, deduplicación, distancia, 52 capitales, radios 5/10/20, vacíos, empate, producto/provincia diferentes, tres fallos, recuperación y cambio de provincia. **No compila ni ejecuta C++, float32 ni ArduinoJson**.
- Primera ejecución `cerca.mjs --red`: pasa. [network-verified.json](network-verified.json): beaconDB 404 con `considerIp:false` y lista vacía, sin redes reales; provincia Madrid/producto 1, HTTP/1.0, Accept JSON, 321529 bytes, 859 estaciones públicas, fecha 09/11/2026 19:23:03. En escritorio el cuerpo se analiza entero para verificar su contrato; eso no es el parser del sketch.
- Dos repeticiones posteriores de la prueba de red agotaron los 15 segundos esperando beaconDB. No sobrescribieron la evidencia anterior. Se registra la variabilidad real del servicio; el sketch trata timeout como intento fallido. La prueba local sin `--red` sigue pasando.
- X2/YE oficiales descargadas por HTTPS de letsencrypt.org, huellas coincidentes con CP/CPS v6.1; ambas autofirmas válidas. YE cruzada por X2: misma clave y firma comprobada con X2. [ca-verified.json](ca-verified.json).
- Conexión TLS real de escritorio a api.beacondb.net con **solo las dos CA del sketch**: autorizada. Cadena observada `*.beacondb.net <- YE2 <- Root YE <- ISRG Root X2` (el servidor también entrega X2 firmado por X1). Sin `setInsecure`, sin certificados inventados ni marcadores pendientes. TLS en ESP32 no comprobado.
- Comando exacto intentado: `"C:\Program Files\Arduino CLI\arduino-cli.exe" compile --fqbn esp32:esp32:esp32 --build-path examples/gasolina-ms-cyd/.build examples/gasolina-ms-cyd/GasolinaMS`. Sale 1 **antes de compilar**: `couldn't read configuration file: open C:\Users\abeli\AppData\Local\Arduino15\arduino-cli.yaml: Acceso denegado.` La política no permite elevar permisos. No se prueba huge_app porque no se ha observado exceso de flash; tamaño nuevo desconocido.
- `.build` no fue creada y se comprueba ausente al terminar. Sin conexión ni escritura en placa.

## Matriz de criterios

| Criterio | Evidencia disponible | Pendiente real |
|---|---|---|
| A1 Escaneo/POST | Revisión del contrato, bloque AP traducido con fixtures, POST vacío real 404 y timeouts posteriores | Escaneo RF manteniendo hotspot y respuesta con posición en ESP32 |
| A2 CA/privacidad | Huellas/firmas y TLS escritorio; revisión de salidas y NVS | TLS ESP32 y observación física de salidas |
| A3 Tres fallos/fallback | Lógica traducida ejecutada, revisión del aviso en footer y CERCA | Transición y reconexión en trayecto real |
| A4 Provincia/cadencias | 52 IDs y capitales verificadas en lógica traducida; revisión de temporizadores y caché | Cruce de provincia y uso durante una hora en placa |
| A5 Streaming/memoria | API provincial real verificada; revisión del parser compartido, Filter, array único, límite y fallo parcial | Ejecutar parser C++, comprobar heap/stack, límite y truncamiento reales |
| A6 CERCA/navegación | Revisión de coordenadas de dibujo, radios/ahorro traducidos, S_COUNT y gfx | Compilación, LCD, gestos y capturas USB 0..4 |
| A7 Regresiones | Config, readList, nameFromCache, handleList, BufferedInput, Strip, sendScreenshot, drawNow, drawTop y drawTrend idénticos por SHA-256; JS token fresco pasa | Repetir prueba física previa tras compilar |
| A8 Documentación/alcance | README y evidencia previa conservada, apartado nuevo y línea pendiente, base respaldada, sin commits/carga, .build ausente | Compilación bloqueada; no se conoce si será necesario huge_app |

## Dudas y límites

Capital más cercana puede elegir otra provincia; no se consultan vecinas. Distancias en línea recta, no rutas. Precisión 50–200 m orientativa, depende de cobertura. Escaneo y HTTPS son síncronos: pueden pausar web/táctil durante operaciones de red. RAM/stack reales y espacio de partición deben comprobarse al desbloquear la compilación. CERCA no tiene evidencia física; la sección Evidencia previa del README sigue describiendo la versión probada en la CYD.
