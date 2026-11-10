# Verificación · Gasolina MS

9 nov 2026. No se ha conectado, enumerado ni grabado ninguna placa. No se ha hecho commit ni push.

## Compilación real intentada

Desde la raíz del repositorio:

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' compile --fqbn esp32:esp32:esp32 --build-path examples/gasolina-ms-cyd/.build examples/gasolina-ms-cyd/GasolinaMS
```

Salida, código 1:

```text
couldn't read configuration file: open C:\Users\abeli\AppData\Local\Arduino15\arduino-cli.yaml: Acceso denegado.
```

Bloqueo del entorno antes de compilar, no diagnóstico de fallo del código. No se intentó elevar permisos ni cambiar la configuración global. No procede Huge APP sin evidencia de desbordamiento de partición. Flash, RAM global y RAM dinámica: **sin medir**. La carpeta `.build` no queda en la entrega.

## Comprobaciones ejecutadas

`node examples/gasolina-ms-cyd/tests/verificar.mjs --red`, código 0:

- Certificado PEM del .ino: sujeto DigiCert Global Root G2, SHA-256 esperado y firma propia válida.
- LICENSE idéntica byte a byte a la de Kilovatio.
- Invariantes de fuente: filtro ArduinoJson, cinco estaciones y catorce muestras, Accept JSON, errata IDPovincia, sin getString ni TLS inseguro, sin lecturas del nombre Wi-Fi para mostrarlo. Solo dos puntos de impresión serie: URL inicial y resumen por intento.
- JavaScript **extraído de la página real** ejecutado en Node con DOM/transporte controlados: carga de selección y 50 L, guardado con token, nombres como texto; lista parcial descartada; respuesta antigua de provincia descartada.
- Red **real** con `Accept: application/json` y como única CA el PEM del sketch: productos, provincias, municipios de Madrid y precios. Para precios, petición TLS HTTP/1.0 real, sin Transfer-Encoding, JSON completo. [Resultado registrado](red-verificada.json).

La referencia de escritorio de Madrid/95 E5 fue: 233 estaciones públicas, mínimo 1,529 €/L, media 1,7160772532 €/L y máximo 1,844 €/L. Cinco menores: 1,529; 1,539; 1,539; 1,539; 1,539. El cálculo de referencia lo ejecuta JavaScript sobre la respuesta completa; **no ejecuta el parser ni los cálculos C++ del firmware**.

## Contraste por criterio de aceptación

| Criterio | Evidencia disponible | Pendiente |
|---|---|---|
| A1, AP y privacidad | Revisión de startPortal, handleRoot, handleWifiSave; AP con QR, contraseña aleatoria; no eco del nombre doméstico. | Alta real desde móvil, reinicio y credenciales en placa. |
| A2, ajustes | Tres escenarios del JavaScript real; servicios oficiales consultados; servidor valida provincia y pertenencia de municipio. | WebServer en ESP32, lectura de QR y persistencia física. |
| A3, cambio de consulta | Revisión: configuración en un blob; cambio de municipio/producto reinicia resumen e histórico; cambio de litros conserva ambos. Identidad del histórico comprobada al cargar. | Ejecución C++ y reinicio durante escritura. |
| A4, flujo y estadísticas | CA y API reales verificadas; readPrices procesa objetos uno a uno con filtro y valida cierre, fecha y OK antes de publicar; addStation mantiene orden estable, suma y extremos sobre todas las P válidas. | Compilar y ejecutar parser/cálculos C++; medir pico de RAM. |
| A5, AHORA/TOP | Revisión de fórmulas `(media − mínimo) × 100` y `(media − mínimo) × litros`, formato con coma, límite de cinco filas, orden ascendente, fecha oficial y ausencia de garantía/apertura. | Lectura y maquetación en LCD. |
| A6, renovación/error | Revisión de intervalos 1 800 000/120 000 ms, botón y estado de error; candidato transaccional; dos únicas impresiones serie; LED actualizado en el bucle. | Caída Wi-Fi/TLS/JSON en placa, tiempos de bloqueo y parpadeo real. |
| A7, tendencia | Revisión: poda a 14 fechas naturales, muestra tras 08:00 solo con fecha oficial de hoy, reemplazo del día; ayer con normalización mktime a mediodía; gráfica no une huecos; caso sin comparación explícito. | Ejecución C++, persistencia, paso de mes/año/DST y uso prolongado. |
| A8, CYD y gestos | Pines, dos buses, rotaciones y calibración proceden de Kilovatio leído completo. Botones con coordenadas y cuerpo hasta y=197. QR con cuatro módulos blancos. | TFT, táctil, brillo, gestos y escaneo físicos. |
| A9, honestidad | Pie íntegro en tres líneas; fuente y límites en web y README; sin coordenadas solicitadas al móvil ni guardadas por el sketch. | Legibilidad del pie en pantalla física. |
| A10, entrega | Un .ino, README español, LICENSE exacta, documentos Spec Kit, pruebas de escritorio y evidencia de red bajo la carpeta solicitada; intento de compilación registrado. | Compilación bloqueada por permisos; prueba física fuera del alcance autorizado. |

## Revisión de casos límite del C++ (sin ejecución)

- Precio vacío, cero, sin coma, negativo, letras o más de tres decimales: rechazado. Coma con una/dos/tres cifras: normalizado a milésimas.
- Venta R u otra distinta de P: excluida antes de sumar. Cero estaciones válidas: estado informativo y LED apagado. De una a cuatro: no se añaden filas ficticias.
- Empates: inserción estable; cuenta y suma incluyen todas las estaciones válidas. Precio inválido no entra en el denominador.
- JSON truncado, lista mal cerrada, duplicación de campos exteriores necesarios, fecha imposible o ResultadoConsulta distinto de OK: el resumen candidato no sustituye al válido.
- Respuesta de fecha anterior: se etiqueta con fecha completa, no se crea muestra de hoy ni comparación engañosa con ayer.
- Sin hora NTP: HTTPS no arranca. Sin ayer: no se compara con otra fecha. Antes de 08:00: no se crea muestra. Reinicio/cambio de producto: identidad del histórico evita mezclas.

Estas revisiones no se presentan como tests superados del firmware. El siguiente paso en un entorno con permisos es compilar el comando indicado y probar el .ino en una CYD cuando su propietario lo autorice.
