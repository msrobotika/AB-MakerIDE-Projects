# Radar Wi-Fi CYD

Firmware Arduino C++ para el perfil declarado CYD ESP32-2432S028R: ILI9341 vertical 240×320 y táctil XPT2046. Todo se dibuja y controla en la placa.

- Al primer arranque, mantén el dedo en cada una de las tres cruces y levántalo cuando se indique. La calibración se guarda. Para repetirla, mantén BOOT durante 1,2 segundos con la aplicación en marcha.
- **RADAR**: barrido naval verde. Una señal más fuerte aparece más cerca del centro. Toca un contacto para abrir el detalle.
- **REDES**: lista por señal con nombre, seguridad, dBm y barras. ANT/SIG cambian de página. Toca una fila para detalles.
- **BUSCAR**: actualiza; también busca automáticamente 8 segundos después de cada resultado. Escaneo pasivo, sin conectarse.

Solo detecta la banda de 2,4 GHz del ESP32. El radio indica intensidad relativa, no metros; el ángulo decorativo se asigna de forma estable por BSSID. Los números de los 12 contactos más fuertes corresponden al orden de la lista. Retiene las 48 señales más fuertes, identifica ocultas y distingue AP con el mismo nombre.

## Compilar

`python scripts/build.py`

Este Python es una utilidad de compilación del PC; el producto es firmware C++ y se ejecuta en la placa. El script prepara una carpeta Arduino con nombre válido y compila el firmware completo; no cambia permisos ni graba. FQBN: `esp32:esp32:jczn_2432s028r`. Core instalado: ESP32 3.3.12. Bibliotecas instaladas: Adafruit GFX 1.13.0, ILI9341 1.6.2, BusIO 1.17.4, XPT2046_Touchscreen 1.4; WiFi, SPI y Preferences del core.

LCD HSPI SCK14/MISO12/MOSI13/CS15/DC2/BL21/RST-1. Táctil SPI separado SCK25/MISO39/MOSI32/CS33/IRQ36. SD CS5 alta, sin montar. No modifica User_Setup.h global.

## Evidencia y respaldo

`specs/001-wifi-radar/` contiene el proceso Spec Kit manual, criterios y evaluación. `evidence/` guarda resultados de compilación, carga y serie. Las autopruebas de lógica se ejecutan en cada arranque y registran RADAR_SELFTEST PASS/FAIL; no prueban la óptica LCD ni la respuesta táctil física.

`backups/pre-spec/` conserva los archivos iniciales con SHA256. `backups/device/original-4MB.bin` contiene la flash original; ver su hash y registro de verificación. Puede contener configuración privada del dispositivo: conservar localmente. No hay restauración automática ni reintento automático de grabación.
