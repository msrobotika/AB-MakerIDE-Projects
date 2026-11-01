# Radar WiFi CYD

**[Descargar proyecto listo para abrir](../../downloads/README.md)**

[← Catálogo](../../README.md) · [Galería](../../docs/GALERIA.md) · [AB MakerIDE SUPER](https://github.com/msrobotika/AB-MakerIDE-SUPER)

Firmware Arduino C++ para el perfil declarado **ESP32-2432S028R**, ILI9341 vertical **240 × 320** y táctil resistivo XPT2046. Los tres archivos de firmware conservan exactamente los bytes aportados.

## Uso

Al primer inicio, mantén el dedo en cada una de las tres cruces y levántalo cuando lo indique. Los coeficientes se guardan en NVS. Mantén BOOT durante 1,2 s con la aplicación en marcha para recalibrar.

| Control | Acción |
|---|---|
| RADAR | Barrido y contactos; toca uno para abrir su detalle. |
| REDES | Lista por señal, cinco redes por página; ANT/SIG navegan y tocar una fila abre el detalle. |
| BUSCAR | Nuevo escaneo; también se repite ocho segundos después del resultado anterior. |
| VOLVER | Regresa desde detalle a la vista anterior. |

Retiene las 48 señales más fuertes y distingue AP por BSSID, incluso si comparten SSID. Las redes ocultas se etiquetan. Solo banda de 2,4 GHz. Intensidad relativa, sin estimación de distancia ni dirección.

## Hardware y pines

| Bus / elemento | Pines GPIO |
|---|---|
| LCD HSPI | SCK 14 · MISO 12 · MOSI 13 · CS 15 · DC 2 · BL 21 · RST -1 |
| Táctil SPI separado | SCK 25 · MISO 39 · MOSI 32 · CS 33 · IRQ 36 |
| microSD | CS 5 en alto; no se monta. |
| BOOT | GPIO 0 |
| LED RGB | GPIO 4, 16, 17 en alto |

Perfil declarado; otras revisiones pueden cambiar panel o pines. [Referencia de pines CYD](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md). No modifica `User_Setup.h` global. El archivo [MAKERIDE_HARDWARE_REFERENCE.txt](MAKERIDE_HARDWARE_REFERENCE.txt) conserva el ejemplo de bloques original como referencia de integración, no como otro firmware de radar.

## Compilar

Entorno de referencia declarado y contrastado en la validación de esta incorporación:

| Componente | Versión |
|---|---|
| Núcleo Arduino ESP32 | 3.3.12 |
| Adafruit GFX Library | 1.13.0 |
| Adafruit ILI9341 | 1.6.2 |
| Adafruit BusIO | 1.17.4 |
| XPT2046_Touchscreen | 1.4 |

WiFi, SPI y Preferences pertenecen al núcleo. Instala Arduino CLI, el núcleo ESP32 y estas bibliotecas por separado. Selecciona **`esp32:esp32:jczn_2432s028r`**. Desde la raíz del repositorio:

```text
python examples/radar-wifi-cyd/scripts/build.py
```

Si Arduino CLI no está en PATH, pasa su ruta con `--arduino-cli`. La utilidad copia los tres fuentes a `.build/RadarCYD/`, renombra el principal a `RadarCYD.ino` y ejecuta una compilación completa. Salidas locales en `.build/output/`, excluidas de Git. No instala herramientas ni carga firmware.

## Evidencia y referencias

[radar_selftest.h](radar_selftest.h) ejecuta helpers de producción al arrancar; informa `RADAR_SELFTEST PASS/FAIL`. Un fallo inhibe la aplicación. Esto no comprueba la óptica del LCD ni la precisión física del táctil. [Resultado de esta incorporación](../../docs/VALIDACION.md).

[MAKERIDE_EXPORT.json](MAKERIDE_EXPORT.json) es la exportación original: estado declarado `flash_reported_verified`, FQBN y puerto declarados. No se ha repetido esa carga. Su identificador de trabajo es metadato del IDE, no credencial.

[README.original.md](reference/README.original.md) se conserva como documento histórico: sus rutas de respaldo, scripts y registros describen el entorno original y no son una promesa de contenido incluido aquí. El script de compilación de este repositorio adapta la utilidad original para localizar Arduino CLI de forma portable. No se incluyen flash, vídeo completo ni registros con rutas personales.
