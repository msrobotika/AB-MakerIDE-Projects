# Kilovatio MS · CYD

**[Descargar proyecto listo para abrir](../../downloads/Kilovatio-MS-CYD.zip)**

[← Catálogo](../../README.md) · [AB MakerIDE SUPER](https://github.com/msrobotika/AB-MakerIDE-SUPER)

Monitor del **precio de la luz (PVPC, península)** para la **CYD ESP32-2432S028R** (ILI9341 horizontal 320 × 240 y táctil resistivo XPT2046). Descarga los precios hora a hora de la API pública de Red Eléctrica y te dice si ahora la luz está barata, normal o cara, cuándo poner la lavadora y cómo vienen los precios de mañana. El LED RGB trasero cambia de color con el precio.

Hecho con AB MakerIDE. Solo usa la placa: Wi-Fi, pantalla, táctil, LED RGB, sensor de luz y altavoz.

## Pantallas

Desliza el dedo a izquierda o derecha para cambiar de pantalla. Los puntos de abajo indican en cuál estás.

| Pantalla | Qué muestra |
|---|---|
| AHORA | Precio de la hora actual en €/kWh, etiqueta BARATA / NORMAL / CARA, posición entre el mínimo y el máximo del día, precio de la hora siguiente y la hora más barata que queda. |
| HOY | Gráfica de 24 barras de colores. Toca una barra para ver su precio. |
| PLANIFICA | Elige Horno (1 h), Lavadora (2 h), Lavavajillas (3 h) o Coche/termo (4 h) y te dice la franja más barata desde ahora, incluida la de mañana si ya está publicada, y cuánto ahorras frente a empezar ya. |
| MAÑANA | Precios del día siguiente. REE los publica cada tarde; la placa los busca sola desde las 20:15. |
| AJUSTES | Estado de la Wi-Fi y de la descarga, brillo automático, LED de precio, aviso sonoro, recalibrar el táctil y cambiar la Wi-Fi (mantén pulsado 2 s). |

| Color (LED y etiquetas) | Significado |
|---|---|
| Verde | La hora está en el tercio más barato del día. |
| Amarillo | Tercio intermedio. |
| Rojo | Tercio más caro. |
| Azul (LED) | Modo de configuración de la Wi-Fi. |

Si activas el aviso sonoro, suenan dos pitidos al empezar la hora más barata del día.

## Primera puesta en marcha

1. Toca el centro de las tres cruces para calibrar el táctil. Se guarda en la placa (NVS) y se puede repetir desde AJUSTES.
2. La placa crea la red **Kilovatio-MS** y muestra un **QR** con una clave nueva en cada arranque del modo de configuración. Escanéalo con el móvil.
3. Se abre la página de configuración (si no, entra en `192.168.4.1`). Elige tu Wi-Fi de 2,4 GHz y escribe la clave.
4. La placa se reinicia, se conecta, sincroniza la hora (Europe/Madrid) y descarga los precios.

La clave de tu Wi-Fi se guarda solo en la memoria de la placa. Para cambiarla, mantén pulsado 2 s el botón rojo de AJUSTES.

## Datos y límites

- Fuente: `apidatos.ree.es`, serie **PVPC (id 1001)** del indicador «Precios mercado peninsular en tiempo real». No necesita cuenta ni clave.
- La conexión HTTPS se valida con las raíces **GlobalSign Root R46** y **GlobalSign Root R3** incluidas en el código. Si REE cambia de certificado raíz, habrá que actualizarlas.
- Es el precio PVPC publicado por REE (energía más peajes y cargos), **sin impuestos ni término de potencia**: sirve para comparar horas, no sustituye a tu factura. Si tienes una tarifa de mercado libre con precio fijo, no te aplica.
- Solo península. Canarias, Baleares, Ceuta y Melilla tienen otras series.
- Hay días con precios negativos en las horas de mucho sol; la pantalla lo marca.

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

1. Abre `KilovatioMS/KilovatioMS.ino` en AB MakerIDE SUPER.
2. Elige la placa **ESP32 Dev Module** (`esp32:esp32:esp32`, núcleo esp32 3.x) y el puerto de tu CYD.
3. Instala las bibliotecas **Adafruit GFX**, **Adafruit ILI9341**, **XPT2046_Touchscreen** (Paul Stoffregen) y **ArduinoJson** 7. El QR usa el generador que ya trae el núcleo esp32.
4. Compila y graba. Cargar reemplaza el programa que tenga la placa.

## Evidencia

- 2 nov 2026: compilado con el núcleo esp32 3.3.12 sin avisos (1 179 639 bytes de programa, 89 %; 52 796 bytes de RAM global, 16 %; 1 179 647 bytes tras la corrección).
- Grabado en una CYD con ESP32-D0WD-V3 por COM4; la escritura se verificó por hash y la placa arrancó con `MSR_KILOVATIO_BOOT`.
- El autor calibró el táctil y configuró la Wi-Fi desde el móvil; la placa respondió `MSR_KILOVATIO_WIFI_OK` y descargó los 24 precios del 2 nov 2026 (mínimo −0,017 y máximo 0,329 €/kWh), los mismos que devuelve la API de REE ese día.
- El autor recorrió las cinco pantallas deslizando (`MSR_KILOVATIO_SCREEN` en el puerto serie).
- El autor grabó un vídeo de 88 s recorriendo las cinco pantallas en la placa. [Capturas de ese vídeo](../../docs/images/kilovatio-ms-cyd/) ([origen y recortes](../../docs/images/kilovatio-ms-cyd/capturas-manifest.json)); se descartan los fotogramas de AJUSTES porque muestran la red Wi-Fi.
- Tras el vídeo se corrigieron dos detalles vistos en la pantalla real: el punto medio de AJUSTES salía como `?` y el texto de los botones activos tenía poco contraste. Se volvió a compilar (1 179 647 bytes) y a grabar con verificación por hash.

No es una validación exhaustiva de todas las revisiones de la CYD ni de todos los casos de la API (por ejemplo, los días de cambio de hora): confirma la serigrafía y la revisión de tu placa antes de grabar.

## Licencia

MIT (ver [LICENSE](LICENSE)). La parte de pantalla y calibración táctil procede de los ejemplos de AB MakerIDE SUPER, también MIT. Las bibliotecas se instalan aparte y conservan sus propias licencias. Los datos de precios son de Red Eléctrica; consulta sus condiciones de uso en [ree.es](https://www.ree.es/es/datos/apidatos).
