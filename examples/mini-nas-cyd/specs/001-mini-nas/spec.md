# Especificación · Mini NAS para CYD
Fecha 2026-10-30. Revisada antes de implementar.
Petición: continuar el servidor NAS. Contexto recuperado de «Proyectos CYD ESP32»: usar la SD y conectarse con el móvil para subir archivos o fotos. No se localiza código NAS previo en las carpetas aportadas; se ha pedido su ruta opcionalmente.

## Historias y aceptación
US1 (P1): con una microSD preparada, conectar el móvil al AP indicado en pantalla, abrir 192.168.4.1 y subir/descargar un archivo sin Internet.
- AC1: firmware completo compilable para esp32:esp32:jczn_2432s028r; LCD HSPI, SD VSPI y táctil por SPI software para evitar conflicto de buses.
- AC2: web móvil autónoma con lista paginada, búsqueda en página, tamaños/espacio, subida secuencial con progreso y cancelación, descarga y vista de imágenes compatibles.
US2 (P1): conservar datos y limitar acceso.
- AC3: WPA2 con contraseña aleatoria de sesión y login maker; token para cambios, nombres validados, carpeta única /MSR_NAS, límite 128 MiB por archivo, sin sobreescritura ni formateo. Upload por streaming a temporal y renombrado solo tras tamaño completo; cancelar/fallo elimina únicamente temporal propio.
- AC4: borrar solo un archivo validado y explícitamente confirmado en la web; sin borrado recursivo. SD ausente/error se informa, sin inventar capacidad ni usar flash como sustituto.
US3 (P2): LCD atractivo y legible con conexión, almacenamiento, clientes y actividad; control de mostrar/ocultar clave y refrescar SD desde pantalla o BOOT.
- AC5: táctil calibrado sin bloquear indefinidamente el arranque; no competir con el bus de SD. La clave no se registra por serial ni se devuelve desde API.
- AC6: fuentes, README, ZIP Arduino válido, instrucciones para IDE y evidencia de compilación/lógica; lista de pruebas físicas pendientes. No afirmar hardware probado.

## Decisiones
Primera versión: AP local, sin Internet, SMB, cuentas múltiples ni acceso remoto. Un cliente HTTP/transferencia a la vez; no prometer sustitución de un NAS doméstico. Carpeta plana; nombres ASCII hasta 80 caracteres, sin puntos/espacios finales, rutas ni nombres reservados. JPEG/PNG/WebP pueden abrirse como imágenes; los demás tipos se descargan como adjuntos.
La clave AP cambia al reiniciar. Se usa la misma clave para usuario maker; HTTP únicamente en el AP WPA2 aislado, sin TLS. El móvil puede pedir permanecer conectado a una red sin Internet. microSD FAT32 preparada por el usuario con la placa apagada; firmware sin formateo ni extracción en caliente certificada.


## Ampliación expresamente autorizada
El autor indica: «crealo y lo subes a la cyd que esta conectada, pero antes sube la meteo estacion al git al repositorio de proyectos». Se ha verificado primero por HTTPS público que weather-station-cyd figura en AB-MakerIDE-Projects con descarga y evidencia. Se autoriza cargar el NAS en la CYD conectada, sin borrar toda la flash ni formatear la SD, y comprobar su funcionamiento. Se conservan fuentes y compilación meteorológica para recuperación; no se extraen volcados de flash/NVS. Las pruebas de escritura se limitan a archivos de ensayo con nombres únicos dentro de /MSR_NAS, con limpieza propia. Se restaura la conexión habitual del PC y se retira cualquier perfil AP temporal utilizado. La clave permanece fuera de repositorios, informes y logs públicos. Se permite diagnóstico serial STATUS con identificador del programa, disponibilidad de SD, capacidad y nombre de AP, sin contraseña ni nombres de archivos personales.
