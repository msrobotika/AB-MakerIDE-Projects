# MacroPad MS · CYD

**[Descargar proyecto listo para abrir](../../downloads/MacroPad-MS-CYD.zip)**

[← Catálogo](../../README.md) · [AB MakerIDE SUPER](https://github.com/msrobotika/AB-MakerIDE-SUPER)

Panel de atajos táctil para la **CYD ESP32-2432S028R** (ILI9341 horizontal 320 × 240 y táctil resistivo XPT2046). Cuatro páginas de seis botones grandes para controlar la música, las reuniones, el trabajo diario y tus directos desde una pantalla de 10 €. Sin Wi-Fi.

Hecho con AB MakerIDE.

## Dos formas de conectarlo

| Conexión | Para quién | Qué necesitas |
|---|---|---|
| **Bluetooth** | PCs, portátiles, tablets y móviles con Bluetooth | Nada más: se empareja como un teclado llamado **MacroPad MS**, sin PIN. |
| **USB** | PCs con Windows sin Bluetooth | El programa [`agent/macropad_agent.py`](agent/macropad_agent.py) (Python 3.9+ y `pip install pyserial`). Lee el cable USB y pulsa las teclas. |

Si los dos están activos, se usa el Bluetooth. Arriba a la derecha la pantalla indica **PC Bluetooth**, **PC por USB** o **Esperando PC**, y el LED parpadea en azul mientras no hay ninguno.

## Páginas

Desliza a izquierda o derecha para cambiar de página. Al tocar un botón se ilumina, suena un clic y el LED destella en verde.

| Página | Botones |
|---|---|
| MULTIMEDIA | Anterior, Play/Pausa, Siguiente, Volumen −, Silencio, Volumen + (el volumen se repite si mantienes pulsado) |
| REUNIONES | Micro Teams (Ctrl+Mayús+M), Cámara Teams (Ctrl+Mayús+O), Mano Teams (Ctrl+Mayús+K), Micro Zoom (Alt+A), Cámara Zoom (Alt+V), Micro Meet (Ctrl+D) |
| TRABAJO | Copiar, Pegar, Deshacer (se repite), Recorte (Win+Mayús+S), Ocultar todo (Win+D), Bloquear PC (Win+L) |
| DIRECTO | Escena 1, Escena 2, Escena 3, Grabar, Directo y Marcar, que envían **F13 a F18**. Asígnalas en OBS (Ajustes → Atajos de teclado) o en tu programa; no chocan con ningún atajo habitual. |
| AJUSTES | Estado de Bluetooth y USB, sonido de clic, brillo automático, recalibrar el táctil y olvidar los PCs emparejados (mantén pulsado 2 s). |

Los atajos de reuniones son los de Windows en Teams, Zoom y Google Meet; el programa tiene que estar en primer plano para recibirlos. Para cambiar un botón, edita la tabla `PAGES` del sketch: cada botón es una etiqueta, una pista, los modificadores y el código HID de la tecla.

## Puesta en marcha

1. Graba el sketch y toca el centro de las tres cruces para calibrar el táctil. Se guarda en la placa.
2. **Bluetooth:** en Windows, Configuración → Bluetooth y dispositivos → Agregar dispositivo → Bluetooth → **MacroPad MS**. En el móvil, ajustes de Bluetooth.
3. **USB:** con la placa conectada, ejecuta `python macropad_agent.py` en la carpeta `agent`. Busca solo el chip CH340 de la CYD; si no lo encuentra, usa `--port COM4`. Con `--dry-run` ves los atajos sin pulsar nada. Ciérralo (Ctrl+C) antes de grabar otro firmware: ocupa el puerto COM.

### Qué puede hacer el programa USB

Solo acepta líneas `MSR_KEY kind=key|media mod=N code=N` y solo pulsa letras con Ctrl, Alt, Mayús o Win, F13–F24 y las teclas multimedia. No ejecuta comandos ni abre archivos. Win+L se hace con la función de bloqueo de Windows, porque Windows no deja simular esa combinación. Las pruebas de `agent/test_macropad_agent.py` comprueban el formato y la lista de teclas (`python -m unittest` en la carpeta `agent`).

## Hardware y pines

| Elemento | Pines GPIO |
|---|---|
| Pantalla ILI9341 (HSPI) | SCLK 14, MISO 12, MOSI 13, CS 15, DC 2, retroiluminación 21 (PWM) |
| Táctil XPT2046 (VSPI) | CLK 25, MISO 39, MOSI 32, CS 33, IRQ 36 |
| LED RGB (activo a nivel bajo) | R 4, G 16, B 17 |
| Sensor de luz (LDR) | 34 |
| Altavoz | 26 |
| microSD | CS 5 se deja en alto; no se monta |

Referencia de pines: [ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md).

## Compilar y cargar

1. Abre `MacroPadMS/MacroPadMS.ino` en AB MakerIDE SUPER.
2. Elige la placa **ESP32 Dev Module** (`esp32:esp32:esp32`, núcleo esp32 3.x) y el puerto de tu CYD.
3. Instala las bibliotecas **Adafruit GFX**, **Adafruit ILI9341**, **XPT2046_Touchscreen** (Paul Stoffregen) y **NimBLE-Arduino** 2.x (h2zero).
4. Compila y graba. Cargar reemplaza el programa que tenga la placa.

**Nota sobre el núcleo esp32 3.x:** al arrancar libera la memoria de Bluetooth si cree que el sketch no la usa, y entonces NimBLE se cuelga. El sketch lo evita declarando `btInUse()`; si copias el código a otro proyecto, conserva esa línea.

## Evidencia

- 2 nov 2026: compilado con el núcleo esp32 3.3.12 y NimBLE-Arduino 2.3.7 sin avisos (660 331 bytes de programa, 50 %).
- Grabado en una CYD con ESP32-D0WD-V3 por COM4 con escritura verificada por hash; arrancó con `MSR_MACROPAD_BOOT` y se anunció como `MacroPad MS` por Bluetooth.
- La primera versión se reiniciaba al arrancar el Bluetooth (aserción en `npl_freertos_mutex_pend`, rastreada con addr2line hasta `NimBLEDevice::createServer`). La causa era la liberación de memoria del núcleo; se corrigió con `btInUse()`.
- El PC de prueba no tiene Bluetooth, así que se probó el **modo USB**: el programa recibió y pulsó 205 atajos tocados por el autor (volumen ±, play/pausa, siguiente, silencio, Win+D, Win+Mayús+S y F16), y se reconectó solo tras dos cortes breves del puerto.
- Las 5 pruebas del programa USB pasan.
- El autor grabó un vídeo de 90 s controlando el PC por USB: pausa, play, volumen arriba y abajo, cambio de página y Win+D. [Capturas de ese vídeo](../../docs/images/macropad-ms-cyd/) ([origen y recortes](../../docs/images/macropad-ms-cyd/capturas-manifest.json)); solo se recorta la placa porque el monitor mostraba un videoclip de terceros.
- El vídeo mostró que «Escritorio» y «Marcador» no cabían en su botón. Se renombraron a «Ocultar todo» y «Marcar», las palabras largas pasan a letra pequeña y se volvió a grabar con verificación por hash (660 339 bytes).

**Pendiente de comprobar:** el emparejamiento Bluetooth con un PC o un móvil reales. No es una validación exhaustiva de todas las revisiones de la CYD: confirma la serigrafía y la revisión de tu placa antes de grabar.

## Licencia

MIT (ver [LICENSE](LICENSE)). La parte de pantalla y calibración táctil procede de los ejemplos de AB MakerIDE SUPER, también MIT. Las bibliotecas se instalan aparte y conservan sus propias licencias. Teams, Zoom, Google Meet, OBS y Windows son marcas de sus propietarios; este proyecto solo envía sus atajos de teclado públicos y no está afiliado a ellos.
