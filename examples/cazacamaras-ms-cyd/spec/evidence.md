# Evidencia y convergencia · CazaCámaras MS

2026-11-09. Implementación terminada; validación de compilación y placa pendiente. No se declara convergencia de los criterios físicos.

## Comprobaciones ejecutadas

- `tests/auditar.mjs`, Node 24.22.1: **15 comprobaciones estáticas superadas**. Comprueba herencia de ayudantes/pines/calibración, licencia exacta, equilibrio de delimitadores, tabla/registro, 20 patrones, pasividad de llamadas, callback protegido, salidas serie, claves NVS, flujo de BUSCAR, reinicio de continuidad y límites de textos literales. No interpreta ni compila C++, ni demuestra funcionamiento del hardware.
- `tests/generar_oui.mjs`: descarga HTTPS de `https://standards-oui.ieee.org/oui/oui.csv`; **128 entradas** verificadas por coincidencia exacta de prefijo y titular, sin duplicados. Extracto en `oui-ieee.json`. Se han consultado además MA-M y MA-S para los nombres sin asignación.
- Lectura completa de Vigía antes del código. Auditoría compara sus ayudantes de texto, táctil, brillo, LED y sonido, así como sus ecuaciones de calibración. El original sigue idéntico a HEAD, normalizando solo finales de línea en esa comparación. LICENSE se compara byte a byte.
- `git diff --stat` sin cambios en archivos seguidos. El único directorio nuevo de esta tarea es `examples/cazacamaras-ms-cyd/`; se conservan los directorios ajenos sin seguimiento que existían al empezar.

## Compilación

Comando ejecutado desde la raíz:

```powershell
& "C:\Program Files\Arduino CLI\arduino-cli.exe" compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --build-path examples/cazacamaras-ms-cyd/.build examples/cazacamaras-ms-cyd/CazaCamarasMS
```

Salida, código de salida **1**:

```text
couldn't read configuration file: open C:\Users\abeli\AppData\Local\Arduino15\arduino-cli.yaml: Acceso denegado.
```

El sandbox impide leer la configuración antes de resolver bibliotecas o compilar. También deniega el acceso al directorio del núcleo instalado. No se han cambiado permisos, instalado herramientas, pedido elevación ni usado una configuración alternativa. **Flash/RAM: no disponibles.** No hay confirmación sobre errores o avisos del compilador. Comprobación final mediante `Test-Path` de la ruta absoluta autorizada: **`.build` ausente**; la CLI no llegó a crear artefactos que retirar.

## Criterios A1–A9

| Criterio | Evidencia disponible | Pendiente |
|---|---|---|
| A1, WiFi y tabla | Revisión de addr2, anillo de 256 con sección crítica, 64 entradas WiFi y ventanas de diez depósitos de un segundo | Recepción real, carga y pérdidas de radio |
| A2, OUI y vídeo | Tabla IEEE, reglas compartidas, subida ToDS sin FromDS/reintentos/nulos, muestras >20 kB/s y >70 % datos durante >10 s; reinicio por pausa/pérdida | Ejecutar `reglas_test.cpp`, contrastar subida y descarga propias en radio; umbral no validado físicamente |
| A3, redes y BLE | 20 patrones completos; SSID solo argumento temporal, etiqueta por índice; `scanNetworks(true,true,true,120,0)` y `setActiveScan(false)` | Hotspot V380-TEST, emisión BLE propia y convivencia de radios |
| A4, semáforo | Revisión de rojo ALTA/vídeo, ámbar con pistas, verde sin ellas, contadores y aviso de fallo | LED y veredicto en la CYD |
| A5, navegación | Seis pantallas, ayudante de gesto de Vigía adaptado, selección de fila con comprobación del número, textos literales dentro de límites | Render real, deslizamiento y toque |
| A6, búsqueda | Canal seleccionado en `radioTick`, cancelación de barrido, suavizado 0,7/0,3 y ritmo 1400–90 ms; silencio tras 6 s sin señal, ignorado en RAM | Intensidad/barra/pitidos y retorno al barrido en placa |
| A7, hardware y ajustes | Pines/ayudantes originales comprobados, Preferences solo sonido/brillo/calibración; radios pausadas al recalibrar | Calibración, persistencia y reinicio reales |
| A8, documentación | README con prueba, privacidad, límites, procedencia y licencia idéntica | Confirmar los pasos físicos descritos |
| A9, compilación y alcance | Comando ejecutado, bloqueo registrado, sin carga, commits ni cambios fuera del destino | Compilar cuando Arduino15 sea accesible; medir flash/RAM |

## Pruebas preparadas, no ejecutadas

`tests/reglas_test.cpp` usa las funciones reales de `Reglas.h` y `TablaOui.h`: límites de 30 s y 20 kB/s, diez segundos sostenidos, salto por trece canales, caducidad, pausas, mayoría de datos, tramas de bajada/nulas/reintentadas, ventana de 10 s, direcciones locales y desbordamiento de `millis()`. No hay compilador C++ de escritorio accesible en PATH ni en las rutas habituales comprobadas. **No se afirma que estas pruebas hayan pasado.**

## Dudas y límites abiertos

- Foscam y Anyka no tienen prefijo incluido: no se ha obtenido una asignación IEEE verificable bajo esos nombres. Detectables solo por las otras pistas. No se han inventado OUI para cubrirlos.
- 8C:85:80 pertenece a Smart Innovation LLC; la relación con Eufy es de un producto observado en CICIoT2023. Imou/Huacheng se contrasta con su contacto oficial. Fuentes en `../OUI.md`.
- Con una radio compartida WiFi/BLE, el tiempo de canal es una estimación superior al tiempo efectivo de recepción. La tasa de vídeo puede quedar infravalorada, y otras subidas pueden parecer vídeo. No demuestra vídeo ni cámara oculta.
- Una tabla acotada y los barridos pasivos pueden omitir dispositivos. Un SSID o nombre BLE puede producir un falso aviso. No detecta SD, cable ni WiFi de 5 GHz.

**Evidencia: pendiente de prueba en la placa.**
