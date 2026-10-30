# Publicación y descarga de proyectos

Fecha: 2026-10-30. Alcance: catálogo público del IDE y descarga del radar existente. La decisión actual del autor sustituye la política anterior de repositorios privados.

## Paquete comprobado

ZIP: `Radar-WiFi-CYD.zip`, seis archivos de fuentes y documentación. SHA-256: `a08c462c2c7edfdba06971e55c44640a043cc70e3cc7e47b4cbccffea9922c86`.

La carpeta `RadarWiFiCYD` contiene el principal `RadarWiFiCYD.ino` y sus dos cabeceras. Los tres contenidos coinciden byte a byte con los originales. El principal solo cambia de nombre para cumplir el formato de Arduino. Incluye README, pines y MIT; no contiene binarios, vídeo ni metadatos de exportación del IDE.

Se ha extraído el ZIP real y compilado esa carpeta completa con Arduino CLI 1.5.1, core ESP32 3.3.12 y las dependencias documentadas: **salida 0**, programa **938.224 bytes (71 %)**, RAM global **50.192 bytes (15 %)**. No se abre puerto ni se graba una placa. La ejecución de autopruebas y las pruebas físicas no se han repetido.

## Privacidad e integridad

Revisados 49 blobs históricos de texto de las bases locales, además de los archivos preparados. Sin patrones conocidos de claves privadas, tokens GitHub/OpenAI ni rutas personales. Esto no equivale a una auditoría exhaustiva de seguridad. Las ocho imágenes siguen siendo los derivados anonimizados ya revisados, sin cambios. Vídeo completo y volcados permanecen fuera de Git.

Enlaces relativos válidos, checksum del catálogo coincidente con el ZIP y manifiesto por archivo. El catálogo contiene enlaces a la fuente canónica; no copia firmware ni imágenes. MIT coherente con el IDE para contenido propio; atribuciones externas conservadas.

## Estado remoto

GitHub confirma público AB-Displays-Lab y la creación de AB-MakerIDE-Projects público. El laboratorio ya figuraba público al comprobar sus ajustes, sin que fuera necesario ejecutar otro cambio de visibilidad. Queda contrastar el contenido de los commits subidos y la descarga sin autenticación al finalizar la entrega.

## Aceptación

| AC | Evidencia | Estado |
|---|---|---|
| AC1 | Laboratorio existente público; historial y contenido revisados, imágenes anonimizadas. | Cumplido en la revisión realizada. |
| AC2 | ZIP extraído, seis entradas, hashes, tres fuentes intactos y compilación real con salida 0. | Cumplido localmente. |
| AC3 | Catálogo público creado; README con proyecto, imagen, descarga, IDE y placa preparado. | Verificación remota de contenido pendiente. |
| AC4 | projects.json enlaza origen canónico y checksum; sin firmware duplicado en catálogo. | Cumplido localmente. |
| AC5 | MIT, atribuciones y guía de contribución. | Cumplido. |
| AC6 | Contraste público de commits y descarga al finalizar. | Pendiente. |

Spec Kit manual con plantillas oficiales locales, constitución revisada y checklist/coherencia antes de implementar. Sin agentes adicionales, releases, cambios al IDE ni publicación en TikTok.
