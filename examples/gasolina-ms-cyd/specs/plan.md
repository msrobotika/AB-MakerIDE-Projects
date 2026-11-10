# Plan · Gasolina MS

Entrada: [spec.md](spec.md). Fecha: 9 nov 2026.

## Contexto técnico

Arduino C++, esp32 3.x, CYD ESP32-2432S028R. Bibliotecas y pines de Kilovatio. Preferences NVS, WebServer/DNSServer, HTTPClient/WiFiClientSecure, ArduinoJson 7. Sin dependencias nuevas ni archivos .h propios.

## Diseño

1. Reutilizar ayudantes de texto, euros, calibración, LED, brillo, QR y gestos de Kilovatio leído completo.
2. HTTPS HTTP/1.0 para evitar flujo chunked; aceptar JSON explícitamente. Analizador de objeto exterior y array incremental, con deserializeJson filtrado por estación; validar cierre, ResultadoConsulta y Fecha antes de publicar el resumen. Presupuesto de memoria independiente del número de estaciones.
3. Listados leídos por elementos con filtro y enviados en JSON al navegador en fragmentos pequeños; navegador monta desplegables con textContent y obtiene nombres oficiales desde opciones validadas por la placa al guardar.
4. Configuración en un blob NVS con versión y límites; histórico en otro blob con identidad de consulta, fecha y hasta catorce muestras. Guardar configuración antes de cambiar estado. Limpiar histórico al cambiar consulta.
5. UI 320×240: cabecera 26 px, cuerpo hasta 196, pie de honestidad de tres líneas y puntos inferiores. Truncar UTF-8 por caracteres visibles. QR con zona blanca de cuatro módulos.
6. Tiempo Europe/Madrid por NTP antes de TLS. Renovación 30 min, reintento en error a los 2 min, además de botón. No operaciones de red desde interrupciones.

## Contratos locales

GET /: portal Wi-Fi en AP, ajustes en LAN. POST /wifi: solo AP. GET /provincias y /municipios?p=ID: JSON de objetos {id,nombre}. GET /config: selección actual. POST /guardar: provincia, municipio, producto, litros; validar pertenencia contra listas oficiales antes de aceptar. Formularios sin recursos externos.

## Control de constitución y coherencia previo

Alcance, preservación, privacidad, evidencia y RAM revisados: conformes. A1–A10 cubiertos por T001–T007. No conflictos pendientes; detalles de muestreo y privacidad de selección Wi-Fi resueltos en especificación. La CLI Arduino ya ha denegado lectura de su configuración: intentar comando exacto y registrar resultado sin elevar permisos.

## Verificación

Pruebas de escritorio de la lógica C++ real cuando el entorno lo permita: orden, exclusión de restringidas, comas, medias y fechas; revisión del parser y casos truncados. Verificar CA mediante SHA-256 conocido y productos en fuente oficial. Intentar compilación solicitada y guardar evidencia. LCD, QR, táctil, Wi-Fi y uso prolongado pendientes en placa.
