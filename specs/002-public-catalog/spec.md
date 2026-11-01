# Especificación: catálogo público y descarga del radar
**Fecha**: 2026-10-30 | **Estado**: revisada antes de implementar

## Objetivo y aclaración
El usuario pide ahora hacer público AB-Displays-Lab y crear un catálogo público de proyectos del IDE, empezando por Radar WiFi. Esta decisión actual sustituye la política anterior de repositorios privados. El objetivo es que quien llegue desde un vídeo encuentre proyecto, descarga e IDE. No se solicita publicar vídeos ni crear una segunda aplicación.

## Historias y criterios
### US1 / P1 — Compartir el proyecto real
- AC1: AB-Displays-Lab existente pasa a público después de revisar contenido e historial; capturas siguen anonimizadas, sin vídeo privado, dumps ni secretos.
- AC2: descarga ZIP de fuentes con nombre Arduino válido, dos cabeceras, guía, referencia y licencia. Su integridad, inventario y compilación se verifican; sin firmware binario ni releases.
### US2 / P1 — Catálogo del IDE
- AC3: crear solo un AB-MakerIDE-Projects público si no existe. README presenta Radar WiFi con imagen real y enlaces a descarga, laboratorio, hardware e IDE.
- AC4: no duplicar el firmware ni capturas en el catálogo; projects.json mantiene origen canónico y checksum del ZIP.
### US3 / P1 — Reutilización y evidencia
- AC5: licencia MIT coherente con el IDE para contenido propio, atribuciones y contribución básicas; mantener condiciones de terceros.
- AC6: verificar rutas, blobs, descarga pública sin sesión, visibilidad de ambos repositorios y cero releases. No afirmar nuevos tests físicos ni firmware distinto.

## Decisiones y límites
Nombre: AB-MakerIDE-Projects. El usuario confirma catálogo público, empezando por Radar WiFi. Licencia MIT igual que el IDE. La fuente canónica del radar permanece en AB-Displays-Lab. Los informes de importación privada se conservan como históricos, con una nota que explica la publicación posterior.
Sin enlaces a una cuenta TikTok no aportada, anuncios externos, seguimiento de visitantes, cuentas nuevas, gastos ni cambios al repositorio del IDE. Las demás aplicaciones privadas conservan su visibilidad.
