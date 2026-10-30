# Especificación: estación meteorológica CYD y vídeo de presentación
Fecha: 2026-10-30. Estado: revisada antes de implementar. Fuente canónica prevista: AB-Displays-Lab/examples/weather-station-cyd/.

## Objetivo confirmado por la petición
Incorporar el proyecto aportado al repositorio público de placas y al catálogo del IDE, con descarga de fuentes. Crear un MP4 vertical editado para TikTok con voz sintética en español, rótulos, enlace/QR y texto de publicación. No publicar en TikTok automáticamente ni crear releases.

## Criterios de aceptación
- AC1: tres fuentes y referencias preservados; README público adaptado, sin rutas personales, credenciales, backups ni firmware compilado. weather_ca.h es un certificado raíz público, no una clave privada.
- AC2: ZIP de fuentes con carpeta Arduino válida, documentación y licencia; incluir proyecto 02 en catálogo conservando radar y sus enlaces.
- AC3: compilación completa real y pruebas de contrato API desde PC identificadas como tales; no afirmar nuevos tests de radio/LCD/táctil ni lecturas meteorológicas vistas en vídeo.
- AC4: revisar los tres vídeos; extraer capturas reales y producir un montaje vertical 1080x1920 con voz, subtítulos, música original suave y CTA a GitHub. Sin redes/contraseñas/localización privada ni monitor privado legibles.
- AC5: identificar la demostración como configuración: los vídeos aportados muestran calibración, portal y estado sin WiFi, no valores meteorológicos recibidos. No inventar datos ni simular una pantalla operativa.
- AC6: verificar visualmente vídeo final, duración/audio, QR, enlaces, ZIP, hashes y commits públicos; entregar MP4, SRT, portada y texto/captura/hashtags útiles.

## Aclaraciones resueltas
Repositorio de placas: AB-Displays-Lab existente público. Catálogo: AB-MakerIDE-Projects. Voz por defecto español de España, sintética, sin imitar a ninguna persona. Duración editorial aproximada 30–35 s. El vídeo completo editado se entrega localmente; los brutos no se suben al repositorio.
La API proporciona datos de modelo Open-Meteo, no medidas de sensores locales. Se conserva atribución y enlace a condiciones; la licencia MIT del código no sustituye las condiciones de la API.

## Ampliación autorizada durante el trabajo

El usuario aporta su red mediante una foto privada y pide probar la placa conectada con Madrid como ejemplo. Se autoriza configurar WiFi/ciudad mediante el portal de la placa y realizar comprobaciones no destructivas. La foto, su QR, SSID, claves, MAC/serie y perfiles del PC no se publican. Se permite una conexión temporal al AP de configuración con restauración de la conexión del PC; no implica autorización de nueva grabación de firmware. Si no es accesible el portal o no hay evidencia de datos recibidos en el ESP32, se informa el límite y no se sustituye por un test desde PC.
