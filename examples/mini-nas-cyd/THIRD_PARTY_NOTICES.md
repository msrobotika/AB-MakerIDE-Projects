# Atribuciones

- [Arduino ESP32 / Espressif](https://github.com/espressif/arduino-esp32), incluidos WiFi, WebServer, SD, SPI y Preferences: licencias de sus componentes, principalmente Apache-2.0/LGPL-2.1 según archivo.
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library), [ILI9341](https://github.com/adafruit/Adafruit_ILI9341) y [BusIO](https://github.com/adafruit/Adafruit_BusIO): licencias BSD de sus repositorios.
- [Referencia CYD de witnessmenow](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md): utilizada para identificar pines. No certifica revisión ni compatibilidad.

No se distribuyen copias de esas bibliotecas gráficas/core. Se incorpora el encoder C de [ricmoo/QRCode](https://github.com/ricmoo/QRCode), versión instalada 0.0.1, copyright Richard Moore 2017, MIT. Véase `src/ricmoo/LICENSE.txt` y `provenance.json`. Se han prefijado cabecera, guardas y símbolos para evitar colisiones con el SDK; el algoritmo no se modifica. La interfaz y sus gráficos CSS no dependen de servicios, fuentes o imágenes externas.
