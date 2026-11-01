# Atribuciones y alcance de licencia

El contenido propio de MS Robotika de este repositorio se comparte bajo [MIT](LICENSE), por decisión actual del autor de publicar estos proyectos open source. Las referencias y dependencias externas conservan sus licencias; no se incluyen sus bibliotecas ni se relicencian.

Radar WiFi CYD usa Arduino ESP32 ([Espressif](https://github.com/espressif/arduino-esp32)), [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library), [Adafruit ILI9341](https://github.com/adafruit/Adafruit_ILI9341), [Adafruit BusIO](https://github.com/adafruit/Adafruit_BusIO) y [XPT2046_Touchscreen de Paul Stoffregen](https://github.com/PaulStoffregen/XPT2046_Touchscreen).

Referencia de pines y variantes CYD: [ESP32 Cheap Yellow Display, witnessmenow](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display). Es una referencia comunitaria; este proyecto no es del fabricante ni es una certificación de compatibilidad con todas las variantes.

El vídeo y proyecto originales fueron aportados por el autor. Las imágenes del radar son fotogramas reales anonimizados; su procedencia se documenta en [Procedencia y privacidad](docs/PROCEDENCIA_Y_PRIVACIDAD.md).

## Estación meteorológica CYD

Usa también [ArduinoJson](https://github.com/bblanchon/ArduinoJson) y [WiFiManager](https://github.com/tzapu/WiFiManager), con sus licencias propias. Los datos son de [Open-Meteo](https://open-meteo.com/) bajo CC BY 4.0; [las condiciones de la API](https://open-meteo.com/en/terms), incluido su uso comercial/promocional, son independientes de la licencia MIT de este código. El certificado de confianza ISRG Root X1 procede de [Let’s Encrypt](https://letsencrypt.org/certificates/) y es público.


## Mini NAS CYD
Usa Arduino ESP32, WiFi/WebServer/SD/SPI/Preferences, Adafruit GFX/ILI9341/BusIO y el encoder C de ricmoo/QRCode 0.0.1 bajo MIT. Este último sí se distribuye con [licencia y procedencia](examples/mini-nas-cyd/src/ricmoo/). [Atribuciones del NAS](examples/mini-nas-cyd/THIRD_PARTY_NOTICES.md). Las cabeceras/símbolos del encoder se prefijan para evitar colisión con el SDK; no se cambió su algoritmo.

El vídeo NAS es un montaje de la grabación aportada, con música original sintetizada para esta edición, sin grabaciones musicales de terceros. Se omiten audio original, QR privados, menús del teléfono y nombres personales. El QR final es un enlace público al IDE. [Edición y procedencia](docs/media/mini-nas-edit-manifest.json).
