# Radar Aviones MS · CYD

[← Catálogo](../../README.md) · [AB MakerIDE SUPER](https://github.com/msrobotika/AB-MakerIDE-SUPER)

Radar en directo de los **aviones que vuelan cerca de ti** para la **CYD ESP32-2432S028R** (ILI9341 horizontal 320 × 240 y táctil resistivo XPT2046). Un radar circular con barrido muestra cada avión en su posición respecto a ti, y una ficha lateral da el vuelo, el modelo, la altura, la velocidad, la distancia y el rumbo.

Hecho con AB MakerIDE. Solo usa la placa: Wi-Fi, pantalla y táctil.

## Cómo se usa

| Gesto | Qué hace |
|---|---|
| Tocar un punto del radar | Elige el avión más cercano al dedo. |
| Tocar la ficha | Pasa al siguiente avión, del más cercano al más lejano. |
| Mantener pulsado 3 s | Abre la configuración de Wi-Fi y ubicación. |

- El radar llega a 25 millas náuticas (unos 46 km), con anillos cada 15 km. El norte está arriba y tú estás en el centro (triángulo azul).
- Los puntos verdes brillan cuando pasa el barrido; los grises están en tierra. El avión elegido sale en naranja, con su rumbo.
- La placa descarga los aviones cada 10 s y, entre descargas, adelanta cada avión según su velocidad y rumbo para que se muevan con suavidad.
- Si pasa un minuto sin datos, el radar se vacía en lugar de mostrar posiciones viejas.

## Primera puesta en marcha

1. Si la placa ya tenía Kilovatio MS, reutiliza su calibración del táctil y su Wi-Fi. Si no, toca el centro de las tres cruces para calibrar.
2. Sin Wi-Fi guardada, la placa crea la red **RadarAviones-MS** y muestra un **QR** con una clave nueva. Escanéalo con el móvil.
3. En la página (si no se abre sola, entra en `192.168.4.1`) escribe tu Wi-Fi de 2,4 GHz y tu **latitud y longitud**. En Google Maps, mantén pulsado tu zona y copia los dos números; con 2 decimales basta (precisión de 1 km).
4. La placa se reinicia y empieza a mostrar aviones. Sin ubicación guardada, usa el centro de Madrid.

Tu Wi-Fi y tus coordenadas se guardan solo en la memoria de la placa. **La pantalla nunca muestra tus coordenadas**, así que puedes grabarla sin revelar dónde vives.

## Datos y límites

- Fuente: API pública de [adsb.lol](https://adsb.lol) (`/v2/point/lat/lon/radio`), una red comunitaria de receptores ADS-B con licencia ODbL. No necesita cuenta ni clave.
- La conexión HTTPS se valida con las raíces **ISRG Root X1** y **ISRG Root X2** (Let's Encrypt) incluidas en el código. Si adsb.lol cambia de certificado raíz, habrá que actualizarlas.
- Solo aparecen los aviones que emiten ADS-B y que algún receptor de la red está oyendo. Algunos vuelos militares o privados no salen.
- Las posiciones llegan con unos segundos de retraso y entre descargas son una estimación. **No sirve para navegación ni para seguridad aérea.**
- La altura es la barométrica que emite el avión, pasada a metros.
- Mientras descarga (alrededor de 1 s cada 10 s), el barrido se detiene un instante.

## Hardware y pines

| Elemento | Pines GPIO |
|---|---|
| Pantalla ILI9341 (HSPI) | SCLK 14, MISO 12, MOSI 13, CS 15, DC 2, retroiluminación 21 |
| Táctil XPT2046 (VSPI) | CLK 25, MISO 39, MOSI 32, CS 33, IRQ 36 |
| LED RGB (activo a nivel bajo) | R 4, G 16, B 17 (azul en modo configuración) |
| microSD | CS 5 se deja en alto; no se monta |

## Compilar y cargar

1. Abre `RadarAvionesMS/RadarAvionesMS.ino` en AB MakerIDE SUPER.
2. Elige la placa **ESP32 Dev Module** (`esp32:esp32:esp32`, núcleo esp32 3.x) y el puerto de tu CYD.
3. Instala las bibliotecas **Adafruit GFX**, **Adafruit ILI9341**, **XPT2046_Touchscreen** (Paul Stoffregen) y **ArduinoJson** 7.
4. Compila y graba. Cargar reemplaza el programa que tenga la placa.

## Evidencia

- 3 nov 2026: compilado con el núcleo esp32 3.3.12 (1 146 004 bytes de programa, 87 %; 53 392 bytes de RAM global, 16 %).
- Grabado en una CYD con ESP32-D0WD-V3 por COM4, con escritura verificada por hash. Al arrancar, la autoprueba de la geometría del radar responde `MSR_RADAR_BOOT selftest=OK`.
- Con la Wi-Fi reutilizada de Kilovatio MS (`MSR_RADAR_WIFI_OK`), descargó entre 32 y 35 aviones cada 10 s alrededor de Madrid (`MSR_RADAR_PLANES n=…`).
- El autor grabó un vídeo de 33 s en la placa: barrido, selección tocando el radar y paso de un avión a otro (AEA84CA, FYS4800 y SAS4303). [Capturas de ese vídeo](../../docs/images/radar-aviones-ms-cyd/) ([origen y recortes](../../docs/images/radar-aviones-ms-cyd/capturas-manifest.json)).
- Tras el vídeo se corrigió un detalle visto en la pantalla real: un texto largo podía dejar píxeles sueltos en el borde derecho. Se volvió a compilar y a grabar con verificación por hash.
- Primeras versiones descartadas en la placa: el lienzo del radar reservado como global dejaba sin memoria el arranque (reinicio en bucle), y adsb.lol rechaza HTTP/1.0 con un 426.

No es una validación de todas las revisiones de la CYD ni de todos los casos de la API: confirma la serigrafía y la revisión de tu placa antes de grabar.

## Licencia

MIT, como el resto de AB MakerIDE.
