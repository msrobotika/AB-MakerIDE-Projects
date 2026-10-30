# Plan de implementación
Lenguaje Arduino C++ (ESP32 core 3.3.12), WebServer/SD/WiFi/Preferences del core, Adafruit GFX/ILI9341. Web HTML/CSS/JS sin CDN, almacenada en flash. Tres fuentes separados: firmware, núcleo de reglas C++ comprobable en compilación y UI incrustada generada a partir del HTML real.

## Investigación y diseño
- Fuente pines: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md
- SD SPI: https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/SD/examples/SD_Test/SD_Test.ino
- Se revisó WebServer.h, Parsing.cpp y RequestHandlersImpl.h del core instalado: callback RAW_START/WRITE/END/ABORTED en POST con application/octet-stream; usar X-File-Name, Content-Length y token en cabeceras, no argumentos de URL durante RAW_START (el parser aún no los procesa).
- LCD HSPI14/12/13/15/DC2/BL21; SD VSPI18/19/23/CS5; XPT2046 software25/39/32/CS33/IRQ36. Calibración de tres puntos, Preferences con namespace distinto del proyecto meteorológico.
- API: GET /api/status, GET /api/files?offset=N, POST /api/upload (binario), GET /api/download?name=N, GET /api/preview?name=N, POST /api/delete?name=N. Todo autenticado; mutaciones con X-NAS-Token.
- Transferencia: validar permiso/nombre/tamaño/espacio, escribir /MSR_NAS/.upload.part, abortar y retirar solo ese temporal ante error, verificar tamaño, cerrar y renombrar al destino sin reemplazo.

## Gate constitucional y coherencia
Pasa: no se modifica fuente anterior, no se formatea/graba hardware, sin datos domésticos, sin publicación automática. AC1→T002/T005; AC2→T003/T005; AC3/4→T002/T004/T005; AC5→T002/T005; AC6→T006. La ausencia de placa probada se conserva como pendiente explícito.


## Ampliación expresamente autorizada
El autor indica: «crealo y lo subes a la cyd que esta conectada, pero antes sube la meteo estacion al git al repositorio de proyectos». Se ha verificado primero por HTTPS público que weather-station-cyd figura en AB-MakerIDE-Projects con descarga y evidencia. Se autoriza cargar el NAS en la CYD conectada, sin borrar toda la flash ni formatear la SD, y comprobar su funcionamiento. Se conservan fuentes y compilación meteorológica para recuperación; no se extraen volcados de flash/NVS. Las pruebas de escritura se limitan a archivos de ensayo con nombres únicos dentro de /MSR_NAS, con limpieza propia. Se restaura la conexión habitual del PC y se retira cualquier perfil AP temporal utilizado. La clave permanece fuera de repositorios, informes y logs públicos. Se permite diagnóstico serial STATUS con identificador del programa, disponibilidad de SD, capacidad y nombre de AP, sin contraseña ni nombres de archivos personales.
