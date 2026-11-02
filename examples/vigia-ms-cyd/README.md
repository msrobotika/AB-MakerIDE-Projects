# Vigía MS · CYD

**[Descargar proyecto listo para abrir](../../downloads/Vigia-MS-CYD.zip)**

[← Catálogo](../../README.md) · [AB MakerIDE SUPER](https://github.com/msrobotika/AB-MakerIDE-SUPER)

Detector de rastreadores Bluetooth para la **CYD ESP32-2432S028R** (ILI9341 horizontal 320 × 240 y táctil resistivo XPT2046). Escucha los anuncios Bluetooth de alrededor y reconoce los rastreadores más comunes. Si uno que no está con su dueño te acompaña durante varios minutos, avisa con la pantalla en rojo, el LED y un pitido. Después te ayuda a encontrarlo con un modo «frío / caliente».

Hecho con AB MakerIDE. Solo usa la placa: sin Wi-Fi, sin cuentas y sin móvil.

## Qué reconoce

| Rastreador | Cómo lo reconoce | ¿Sabe si está lejos de su dueño? |
|---|---|---|
| Apple Find My (AirTag y accesorios compatibles) | Datos de fabricante de Apple (0x004C) con anuncio Find My (0x12) | Sí: anuncio largo = lejos de su dueño; corto = con su dueño |
| Samsung SmartTag | Datos de servicio 0xFD5A | No: estado desconocido |
| Tile | Servicio 0xFEED o 0xFEEC | No: estado desconocido |
| Red Encontrar de Google (Find Hub) | Datos de servicio 0xFEAA, trama 0x40 o 0x41 | Sí, si el rastreador está en modo de protección (0x41) |

Solo avisan los que están **lejos de su dueño** o en **estado desconocido**. Los que están con su dueño cerca (por ejemplo, los AirPods o el AirTag de un vecino en su casa) se muestran, pero no hacen saltar la alarma.

## Pantallas

Desliza a izquierda o derecha para cambiar de pantalla.

| Pantalla | Qué muestra |
|---|---|
| VIGILANCIA | Semáforo: «Todo tranquilo», «N sin su dueño» o «¡ATENCIÓN!», con el recuento por marca y cuántos dispositivos Bluetooth hay alrededor. |
| RADAR | Cada rastreador es un punto. Cuanto más fuerte es su señal, más cerca del centro. El ángulo es decorativo: con una sola antena no se sabe la dirección. |
| LISTA | Hasta seis rastreadores con su marca, si están con su dueño o lejos, los minutos que llevan cerca y la intensidad de la señal. Toca uno para buscarlo. |
| BUSCAR | «FRÍO / TEMPLADO / ¡CALIENTE!» con una barra y pitidos que se aceleran al acercarte. El botón «Es mío, no avisar» lo excluye de la alarma hasta reiniciar. |
| AJUSTES | Minutos hasta la alarma (1, 5, 10 o 20), sonido, brillo automático y recalibrar el táctil. |

| LED | Significado |
|---|---|
| Latido verde | Tranquilo, sin rastreadores cerca |
| Latido azul | Hay rastreadores cerca |
| Rojo parpadeando | Alarma |

## Privacidad y límites

- **Solo escucha.** El escaneo es pasivo: no se conecta a ningún dispositivo ni envía nada. Tampoco guarda nada: al reiniciar, la lista empieza de cero.
- **No muestra direcciones.** En pantalla y por el puerto serie cada rastreador es un número corto (#1, #2…), nunca su dirección Bluetooth.
- **No detecta todos los rastreadores.** Los rastreadores cambian de dirección Bluetooth para proteger la privacidad de su dueño, así que el tiempo «cerca de ti» puede reiniciarse. Algunos modelos usan formatos que este proyecto no reconoce.
- **No sustituye** a las alertas de rastreo no deseado de iOS y Android. Si crees que te están siguiendo, busca ayuda: en España, el **112**.
- El radar y la distancia son aproximados: la señal Bluetooth cambia con paredes, bolsos y el propio cuerpo.

## Probarlo sin un AirTag

Con un móvil Android y la app gratuita **nRF Connect for Mobile** (Nordic Semiconductor) se puede emitir la misma señal que un rastreador:

1. En la placa, AJUSTES → «Alarma» hasta **1 min**.
2. En nRF Connect: **ADVERTISER → + → ADD RECORD → Manufacturer Data**, Company ID `0x004C`, Data `12191000`. Activa el anuncio.
3. En unos segundos la LISTA muestra «Apple Find My · LEJOS de su dueño» y al minuto salta la alarma.
4. Para Samsung: **ADD RECORD → Service Data**, UUID `FD5A`, datos `00`.

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

1. Abre `VigiaMS/VigiaMS.ino` en AB MakerIDE SUPER.
2. Elige la placa **ESP32 Dev Module** (`esp32:esp32:esp32`, núcleo esp32 3.x) y el puerto de tu CYD.
3. Instala las bibliotecas **Adafruit GFX**, **Adafruit ILI9341**, **XPT2046_Touchscreen** (Paul Stoffregen) y **NimBLE-Arduino** 2.x (h2zero).
4. Compila y graba. Cargar reemplaza el programa que tenga la placa. La primera vez, toca las tres cruces para calibrar el táctil; se guarda en la placa.

El sketch declara `btInUse()` para que el núcleo esp32 3.x no libere la memoria de Bluetooth al arrancar; si copias el código a otro proyecto, conserva esa línea.

## Evidencia

- 2 nov 2026: compilado con el núcleo esp32 3.3.12 y NimBLE-Arduino 2.3.7 sin avisos (663 891 bytes de programa en la versión final, 50 %).
- Grabado en una CYD con ESP32-D0WD-V3 por COM4 con escritura verificada por hash; arrancó con `MSR_VIGIA_BOOT` y empezó el escaneo pasivo.
- En casa del autor detectó **dispositivos Apple Find My reales** (hasta dos a la vez) con su dueño cerca, con señal de −97 a −103 dBm, y entre 18 y 20 dispositivos Bluetooth en total. El autor grabó un vídeo de 32 s recorriendo las cinco pantallas. [Capturas de ese vídeo](../../docs/images/vigia-ms-cyd/) ([origen y recortes](../../docs/images/vigia-ms-cyd/capturas-manifest.json)).
- Tras el vídeo, la pantalla VIGILANCIA dejó de avisar en amarillo por rastreadores que están con su dueño, se acortó un titular que no cabía y la leyenda del RADAR pasó a nombres cortos porque se cortaba por la derecha.

**Pendiente de comprobar en el registro de la placa:** la detección de un rastreador lejos de su dueño y la alarma, con un AirTag real o con la simulación de nRF Connect descrita arriba. No es una validación exhaustiva de todas las revisiones de la CYD ni de todos los modelos de rastreador.

## Licencia

MIT (ver [LICENSE](LICENSE)). La parte de pantalla y calibración táctil procede de los ejemplos de AB MakerIDE SUPER, también MIT. Las bibliotecas se instalan aparte y conservan sus propias licencias. AirTag, Find My, SmartTag, Tile y Google son marcas de sus propietarios; este proyecto solo reconoce sus anuncios Bluetooth públicos y no está afiliado a ellos.
