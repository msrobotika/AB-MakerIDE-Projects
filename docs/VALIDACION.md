# Validación de la incorporación

> Informe histórico de la importación inicial privada. La publicación pública y el ZIP posterior se documentan en [la verificación de publicación](../specs/002-public-catalog/verification.md).

Fecha: **2026-10-30**. Alcance: proyecto aportado y documentación de AB Displays Lab. No se ha accedido a un dispositivo físico.

## Compilación completa ejecutada

Comando desde la raíz: `python examples/radar-wifi-cyd/scripts/build.py`.

- Arduino CLI **1.5.1**, commit `01f3d4f2b`.
- Núcleo ESP32 **3.3.12**; FQBN **esp32:esp32:jczn_2432s028r**.
- Adafruit GFX **1.13.0**, ILI9341 **1.6.2**, BusIO **1.17.4**, XPT2046_Touchscreen **1.4**, instaladas y consultadas en este equipo.
- **Salida 0**, compilación del sketch completo y sus cabeceras.
- Programa: **938.224 / 1.310.720 bytes (71 %)**.
- Variables globales: **50.192 / 327.680 bytes (15 %)**; disponibles para variables locales: **277.488 bytes**.

Salida de Arduino CLI, sin rutas del equipo:

```text
Sketch uses 938224 bytes (71%) of program storage space. Maximum is 1310720 bytes.
Global variables use 50192 bytes (15%) of dynamic memory, leaving 277488 bytes for local variables. Maximum is 327680 bytes.
```

La utilidad prepara una carpeta Arduino válida, sin cambiar los originales ni instalar o grabar. Sus opciones se han consultado con `--help`. Los binarios, ELF y mapas generados permanecen excluidos de Git.

## Integridad y documentación

Resultados automatizados en [validation-results.json](validation-results.json): seis archivos originales con bytes y SHA-256 coincidentes; ocho PNG decodificables y con hashes correctos; comparación píxel a píxel contra el vídeo, salvo las máscaras opacas y la leyenda exterior; enlaces relativos resueltos. Revisión visual de los ocho PNG completada: sin SSID/BSSID legibles ni monitor de fondo en los recortes.

Se revisa el inventario y se buscan formatos comunes de claves, tokens, contraseñas y rutas personales. Esta búsqueda no prueba la ausencia de todo secreto posible. No se incluyen vídeo, volcados de flash, binarios compilados ni registros privados.

## Qué no se ha probado aquí

No se ejecutan tests de host de las cabeceras: no hay compilador C++ nativo configurado para esta comprobación. Las autopruebas de `radar_selftest.h` se compilan dentro del firmware; **no se afirma un PASS de ejecución**. Tampoco se han repetido carga, monitor serie, calibración física, reinicios o escaneos en una placa. El vídeo y la exportación se describen por separado en [EVIDENCIA.md](EVIDENCIA.md).

## Aceptación de la incorporación

| Criterio | Resultado y evidencia |
|---|---|
| AC1 | Repositorio existente identificado como privado y vacío antes de incorporar; sin crear duplicados ni releases. Visibilidad privada y cero releases confirmados tras el push. |
| AC2 | Seis originales byte a byte; [source-manifest.json](source-manifest.json). |
| AC3 | README, guía de compilación y pines; enlaces al IDE y su guía consultados en GitHub. Compilación completa con la utilidad incluida. |
| AC4 | Ocho PNG de una nueva extracción real; cuadros, tiempos, máscaras y hashes en [manifest.json](images/radar-wifi-cyd/manifest.json). |
| AC5 | Revisión visual y del inventario; recorte LCD y ocultación sólida de identificadores. |
| AC6 | Compilación actual, vídeo físico, exportación histórica y autopruebas no ejecutadas expresamente separados. |
| AC7 | Hashes y enlaces internos contrastados; 36 rutas y SHA de blob coinciden con el commit remoto a7e389f. |

El alcance de aceptación es importar y documentar el proyecto real; las pruebas físicas pendientes no se marcan superadas.

[Verificación remota y aceptación por criterio](../specs/001-import-radar/verification.md).
