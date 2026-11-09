# Plan · CazaCámaras MS

2026-11-09 · Referencia: [spec.md](spec.md).

## Contexto técnico

Arduino C++ para ESP32 3.x, NimBLE-Arduino 2.x, Adafruit GFX/ILI9341, XPT2046, Preferences y WiFi/esp_wifi del núcleo. CYD de 4 MB, partición `huge_app`. No añadir bibliotecas externas.

## Diseño

1. Mantener ayudantes originales de Vigía para texto, botones, táctil, LED, brillo, pitidos y gesto. Modificar solo cuerpos relacionados con el producto.
2. Separar en pestañas `.h` las reglas puras y la tabla OUI. Registro IEEE descargado en desarrollo; firmware sin Internet ni actualizaciones. Guardar extracto verificable y tabla legible.
3. Callback WiFi valida cabecera y copia addr2, longitud, RSSI, tipo, canal, subida y hora a un anillo fijo con sección crítica. BLE usa cola FreeRTOS. Toda tabla de detecciones pertenece a `loop()`.
4. Diez depósitos por segundo para tráfico real. Acumular tiempo de escucha por canal; evaluar muestras por visita (o intervalos de 250 ms en BUSCAR). Más de 70 % datos, subida no nula y tasa >20 kB/s. Excluir reintentos, tramas nulas y bajada de AP del cálculo de subida. No deducir una cámara por una sola ráfaga.
5. Escaneo de redes asíncrono, pasivo y corto; limpiar resultados al terminar, restaurar promiscuo. BUSCAR cancela/evita barridos de redes. Calibración pausa ambas radios. Errores y colas saturadas se comunican en AJUSTES.
6. Hasta 64 WiFi (incluidas las redes, fusionadas por BSSID con el dispositivo WiFi cuando existe) y 16 pistas BLE. Números efímeros, ignorado en RAM. No guardar nombres ni paquetes.

## Puertas de constitución

Antes de implementar: alcance, pasividad, datos efímeros y herencia de Vigía revisados. No hay requisito pendiente de una respuesta del usuario. OUI sin asignación segura se excluye y se declara en README. Después: revisión de API local/primaria, pruebas reproducibles y evidencia por A1–A9.

## Estructura y verificación

`CazaCamarasMS/CazaCamarasMS.ino`, pestañas de reglas/OUI, `README.md`, `LICENSE`, `spec/` y `tests/`. Solo esta carpeta. Pruebas de lógica ejecutando el C++ real si hay compilador accesible; si no, indicar límite y realizar auditoría automática de estructura/privacidad/prefijos. Compilación Arduino con el comando exacto del usuario; sin placa conectada ni subida. Eliminar `.build` comprobando su ruta absoluta.
