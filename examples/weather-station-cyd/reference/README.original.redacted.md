> Documento histórico. Se ha ocultado una ruta personal y redirigido el enlace de verificación al informe de esta incorporación.

# Estación meteorológica Wi-Fi — MS Robotika

Firmware Arduino/C++ para el perfil **CYD ESP32-2432S028R original**, ILI9341 240 × 320 y táctil resistivo XPT2046. Placa declarada por el proyecto, pendiente de comprobar físicamente. FQBN `esp32:esp32:jczn_2432s028r`; puerto declarado **COM4**. No se ha abierto ni grabado ese puerto.

Muestra temperatura, sensación, humedad, viento, presión al nivel del mar, precipitación del intervalo actual del proveedor, estado del cielo y previsión de hoy y los dos días siguientes (mínima/máxima y probabilidad máxima diaria de precipitación). Son **datos de modelo de Open-Meteo**, no medidas de sensores locales.

## Primer uso, después de una carga autorizada

1. Alimenta la placa. La primera vez toca las tres cruces para calibrar; se conserva en NVS. Tras 15 segundos sin completar un punto, se omite la calibración y siguen disponibles los gestos y BOOT.
2. La pantalla muestra la red **MSR-Meteo-XXXX**, su contraseña aleatoria de sesión y **http://192.168.4.1**. Conecta el móvil/PC a esa red; acepta permanecer en ella aunque no tenga Internet. Si no se abre el portal automáticamente, escribe esa dirección.
3. En **Configure WiFi**, introduce la red doméstica **de 2,4 GHz**, su contraseña y tu ciudad. Opcionalmente indica país de dos letras (`ES`, `MX`, `AR`…). No hay ciudad predeterminada ni clave de API.
4. Guarda. La placa conecta, sincroniza la hora por NTP y consulta Open-Meteo mediante HTTPS con validación de certificado. En la siguiente puesta en marcha usa la configuración guardada.

El servicio de geocodificación elige el primer resultado y el LCD muestra nombre, región y país. Para ciudades homónimas, refina la búsqueda con la región tras una coma y/o el código de país. Las localidades inexistentes generan un aviso; no se sustituyen por otra ubicación predeterminada.

El portal es únicamente auxiliar de configuración. Los datos meteorológicos se dibujan en la **pantalla de la placa**, sin aplicación ni servidor PC.

## Controles y recuperación

- **ACTUALIZAR** o toque breve: consulta manual, con un mínimo de 60 segundos entre intentos.
- **WIFI**: abre o cierra el portal usando la calibración guardada.
- Mantener la pantalla o **BOOT** durante 2 segundos: abre/cierra el portal, incluso sin calibración. BOOT breve también actualiza. No mantener BOOT mientras se reinicia/alimenta: es el botón de arranque del ESP32.
- Actualización automática cada **15 minutos**; tras fallo, reintento cada **2 minutos**. Ante pérdida de red conserva lo recibido y el stack intenta reconectar. Al pasar un minuto desconectado, abre el portal de recuperación sin borrar las credenciales.
- La línea inferior muestra minutos desde la última **recepción** y marca **ANTIGUO** a partir de 30 minutos. La fecha/hora del dato corresponde a la ubicación y se muestra por separado. Un reinicio no conserva el tiempo meteorológico previo: espera nuevos datos.

El AP permanece abierto hasta cerrar el portal o guardar la conexión correctamente. La contraseña del AP cambia al reiniciar. Los datos Wi-Fi y ubicación persisten en la NVS del dispositivo; no se imprimen por serie ni se incrustan en el código. Esta versión no activa cifrado de flash/NVS. Si falla la hora NTP o cambia la cadena de certificados del proveedor, se muestra error y se reintenta; no se desactiva la validación HTTPS.

## Pines y dependencias

| Función | GPIO |
|---|---|
| LCD HSPI SCLK / MISO / MOSI | 14 / 12 / 13 |
| LCD CS / DC / BL / RST | 15 / 2 / 21 / -1 (reset placa) |
| XPT2046 VSPI CLK / MISO / MOSI | 25 / 39 / 32 |
| XPT2046 CS / IRQ | 33 / 36 |
| SD CS | 5, nivel alto; SD sin inicializar |
| BOOT | 0, entrada con pull-up |

Los GPIO 36 y 39 solo se usan como entradas. No se inicializan SD, altavoz ni sensores externos. No se modifica `User_Setup.h` ni ajustes globales de otras pantallas. Inicialización y calibración afín proceden del ejemplo de `MAKERIDE_HARDWARE_REFERENCE.txt`.

Dependencias existentes utilizadas: ESP32 **3.3.12**, Adafruit GFX **1.13.0**, Adafruit ILI9341 **1.6.2**, Adafruit BusIO **1.17.4**, XPT2046_Touchscreen **1.4**, ArduinoJson **7.4.2**, WiFiManager **2.0.17**. WiFi, HTTPClient, NetworkClientSecure, Preferences y FreeRTOS vienen con el core ESP32. No se han instalado librerías.

## Compilar y comprobar desde el PC

```powershell
python tools/build.py
python tests/test_api_contract.py
```

`tools/build.py` copia el sketch y sus cabeceras a `build/station/station.ino`, compila el firmware **completo** con el FQBN declarado y deja los binarios en `build/firmware`. No contiene órdenes de carga. Se usa Python solo para herramientas de desarrollo del PC; la aplicación de destino es firmware C++ para el ESP32.

Las pruebas consultan realmente las APIs desde el PC con la misma raíz TLS del firmware, contrastan unidades/campos/previsión, comprueban una localidad inexistente y el respaldo inicial. Sevilla es un dato de prueba, no la ubicación del usuario. Los JSON de esas consultas se conservan como evidencia en `tests/fixtures/`. Estas pruebas no ejecutan el firmware ni comprueban Wi-Fi/pantalla/táctil físicos.

La tarea FreeRTOS de red no dibuja ni modifica configuración compartida: recibe peticiones y devuelve resultados mediante colas. Un contador de generación descarta resultados de una ubicación anterior. La pantalla y los controles se atienden en el bucle principal; WiFiManager puede esperar hasta 10 segundos al conectar/guardar una red. Se limita la frecuencia de consultas incluso al cambiar ubicación.

## Referencias y condiciones

- [Pines CYD](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md).
- [API meteorológica](https://open-meteo.com/en/docs) y [geocodificación](https://open-meteo.com/en/docs/geocoding-api).
- [Condiciones Open-Meteo](https://open-meteo.com/en/terms): API gratuita para uso no comercial; menos de 10.000 solicitudes/día, 5.000/hora y 600/minuto. Datos bajo CC BY 4.0; atribución visible en pantalla. La frecuencia normal es de unas 96 consultas meteorológicas al día.
- [Raíces oficiales Let's Encrypt](https://letsencrypt.org/certificates/). `weather_ca.h` contiene ISRG Root X1 obtenida del dominio oficial y validada en conexiones reales desde PC.

Se ha aplicado **Spec Kit manualmente**, con plantillas oficiales de `[copia local de GitHub Spec Kit]`, porque la CLI no está disponible. Documentación en `.specify/memory/constitution.md` y `specs/001-estacion-meteorologica/`. Respaldo inicial verificado en `backup-inicial/`.

La evidencia final y las comprobaciones físicas pendientes se detallan en [verification.md](../../../specs/003-weather-station/verification.md).
