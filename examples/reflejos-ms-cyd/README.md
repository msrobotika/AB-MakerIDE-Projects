# Reflejos MS · CYD

**[Descargar proyecto listo para abrir](../../downloads/Reflejos-MS-CYD.zip)**

[← Catálogo](../../README.md) · [AB MakerIDE SUPER](https://github.com/msrobotika/AB-MakerIDE-SUPER)

Juego original de reflejos para la **CYD ESP32-2432S028R** (ILI9341 vertical 240 × 320 y táctil resistivo XPT2046). En una placa de circuito dibujada se enciende un chip: tócalo antes de que se vacíe la barra de tiempo. Cada acierto acelera el ritmo y el LED RGB trasero se ilumina en verde (acierto) o rojo (fallo).

Sin Wi-Fi, microSD, altavoz ni archivos externos. Hecho con AB MakerIDE.

## Cómo se juega

| Elemento | Qué hace |
|---|---|
| Chip en cian | Es el objetivo: tócalo antes de que se acabe la barra. |
| Barra de tiempo | Verde, amarilla y roja según el tiempo que queda; empieza en 1,3 s y baja hasta 0,38 s. |
| PUNTOS / RACHA | Cada acierto suma 10 puntos más 2 por cada acierto seguido (hasta 20). |
| Vidas | Tres. Se pierde una al tocar otro chip o si se acaba el tiempo. |
| RÉCORD | Mejor puntuación desde el último encendido (se guarda solo en RAM). |
| LED RGB trasero | Verde al acertar, rojo al fallar, azul al arrancar. |

Al encender, toca el centro de las tres cruces para calibrar el táctil. La calibración se guarda solo en RAM y se repite tras cada reinicio.

## Hardware y pines

| Elemento | Pines GPIO |
|---|---|
| Pantalla ILI9341 (HSPI) | SCLK 14, MISO 12, MOSI 13, CS 15, DC 2, retroiluminación 21 |
| Táctil XPT2046 (VSPI) | CLK 25, MISO 39, MOSI 32, CS 33, IRQ 36 |
| LED RGB (activo a nivel bajo) | R 4, G 16, B 17 |
| microSD | CS 5 se deja en alto; no se monta |

Referencia de pines: [ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md).

## Compilar y cargar

1. Abre `ReflejosMS/ReflejosMS.ino` en AB MakerIDE SUPER.
2. Elige la placa **ESP32 Dev Module** (`esp32:esp32:esp32`) y el puerto de tu CYD.
3. Instala las bibliotecas **Adafruit GFX**, **Adafruit ILI9341** y **XPT2046_Touchscreen** (Paul Stoffregen).
4. Compila y graba. Cargar reemplaza el programa que tenga la placa.

## Evidencia

- 1 nov 2026: compilado con el núcleo esp32 3.3.12 (316 927 bytes de programa, 24 %; 24 492 bytes de RAM global, 7 %).
- Grabado en una CYD con ESP32-D0WD-V3 por COM4; la escritura se verificó por hash y la placa arrancó con `MSR_REFLEJOS_BOOT`.
- El autor confirmó en la placa que la pantalla y el táctil funcionan y grabó un vídeo de 12 s jugando. [Capturas de ese vídeo](../../docs/images/reflejos-ms-cyd/) ([origen y recortes](../../docs/images/reflejos-ms-cyd/capturas-manifest.json)).

No es una validación exhaustiva de todas las revisiones de la CYD: confirma la serigrafía y la revisión de tu placa antes de grabar.

## Licencia

MIT (ver [LICENSE](LICENSE)). La parte de pantalla y calibración táctil procede de los ejemplos de AB MakerIDE SUPER, también MIT. Las bibliotecas se instalan aparte y conservan sus propias licencias.
