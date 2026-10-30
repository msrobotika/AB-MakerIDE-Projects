# Acceso QR al Mini NAS
2026-10-30. Petición: «no veo esa clave, mejor por qr». Mejora y nueva carga en la CYD conectada autorizadas por continuidad de la tarea. Base respaldada por commit db2d23d; proceso Spec Kit manual, plantillas oficiales y constitución existente. Sin publicación adicional ni releases.

## Criterios
- AC1: arrancar con QR WiFi WPA2 que contiene el SSID y clave de la sesión actual, negro/blanco, zona libre de cuatro módulos, tamaño legible en 240×320; no se oculta por temporizador.
- AC2: botón QR WEB permite un segundo escaneo para entrar al panel sin escribir credenciales. Token aleatorio de 128 bits, único uso, caducidad de 5 minutos; fragmento URL retirado por JS antes de solicitar sesión. Cookie HttpOnly SameSite=Strict, solo en la RAM de esta sesión. API conserva autenticación; páginas de arranque públicas no devuelven datos privados. Alternativa Basic manual conservada.
- AC3: controles QR WIFI / QR WEB / ESTADO; BOOT breve muestra QR WiFi, largo mantiene recuperación de calibración. Pantalla de estado conserva SD y calibración.
- AC4: las funciones de archivos, no reemplazo, token de escritura, SD y calibración se preservan. No registrar ni empaquetar códigos/contraseñas/tokens reales.
- AC5: compilar/cargar, decodificar matrices producidas por el encoder real con datos de prueba públicos, probar bootstrap JS y reglas de token, confirmar arranque/SD. Diferenciar esas comprobaciones de la lectura real con cámara y acceso móvil, que requieren confirmación del autor.

## Plan y decisiones
QRCode ricmoo 0.0.1 instalado; QR versión 5, ECC medium, módulo de 4 px, borde 4 módulos: 180×180. WiFi emplea formato WIFI:T:WPA;S:...;P:...;; con campos actuales limitados a ASCII seguro. Segundo QR: http://192.168.4.1/?pair=1#pair=<token>. Solo las cabeceras POST transportan el token al servidor; no incluir credenciales Basic en enlaces. Cookie de sesión local sin Secure porque el AP sirve HTTP. Leer Cookie y X-NAS-Pair; /api/pair valida token vigente y lo consume antes de responder. El QR web caducado se renueva al abrir la pantalla; las sesiones ya concedidas siguen válidas hasta reiniciar. Una sesión por arranque compartida para los escaneos físicos autorizados. Sin CORS, CSRF de escrituras conservado.

Fuentes consultadas: https://github.com/ricmoo/QRCode · https://github.com/zxing/zxing/wiki/Barcode-Contents · https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Set-Cookie

## Tareas y checklist previo
- [x] T001 Base comprometida, alcance/seguridad aclarados; criterios enlazados al plan; permisos de carga existentes revisados.
- [x] T002 Encoder, pantallas QR y controles; flujo de sesión y bootstrap.
- [x] T003 Pruebas de reglas/caducidad/replay y bootstrap JS; compilación completa.
- [x] T004 Carga autorizada, diagnóstico y decodificación QR de prueba desde dispositivo; confirmación móvil si está disponible.
- [x] T005 Actualizar guía y ZIP con evidencia honesta y privacidad.

Coherencia: AC1/3→pantallas y encoder; AC2→sesión/token y bootstrap; AC4→regresión existente; AC5→compilación, matrices de dispositivo y evidencia separada. No hay incógnitas bloqueantes. El QR WiFi y QR de acceso reales son credenciales y no se publican.

## Resolución de dependencia antes de continuar
La primera compilación encontró el qrcode.h del SDK en lugar del de ricmoo, sin seleccionar QRCode como biblioteca. Se incorpora el encoder C de ricmoo 0.0.1 bajo src/ricmoo, conservando su licencia MIT, con cabecera y símbolos prefijados para eliminar la colisión. Solo cambian nombres/include/guardas; el algoritmo se contrasta mediante decodificación de matrices ejecutadas en la CYD. Se registran hashes de archivos originales y adaptados. No se instala ninguna biblioteca ni se modifica el SDK o la instalación global de Arduino.
