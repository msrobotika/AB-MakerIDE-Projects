# Publicar Mini NAS CYD y consolidar proyectos
Fecha: 2026-10-31. Base: catálogo 79f08b4, laboratorio 3219422 y NAS c30e47c, con respaldo git bundle local de los tres. Spec Kit manual con plantillas oficiales locales; CLI specify no disponible. Se aplica la constitución existente con ampliación actual de publicación.

## Petición y alcance
El autor pide subir el NAS al repositorio de proyectos como las otras dos demostraciones y eliminar «el del radar solo». El único candidato encontrado es AB-Displays-Lab, que actualmente guarda radar y meteo. Se ha solicitado aclaración; hasta resolverla no se eliminará ese repositorio. La publicación del NAS en AB-MakerIDE-Projects sí está autorizada.

## Criterios de aceptación
- AC1: Mini NAS en examples/mini-nas-cyd, guía, licencia, pines, dependencias y fuentes idénticas a la carga corregida a 1 MHz; ZIP Arduino con principal MiniNasCYD.ino, cinco cabeceras y src/ricmoo con licencia/procedencia.
- AC2: README e índice reúnen tres proyectos con enlaces reales de código, descarga, evidencia e IDE. Sin repositorios nuevos ni releases.
- AC3: incorporar vídeo editado de 30 s y capturas derivadas del original; excluir QR/clave de sesión, datos del teléfono, nombres de archivos personales y grabación íntegra. Música original y montaje identificados.
- AC4: separar compilación/carga, seis pruebas reales de SD y confirmación del móvil de tests JS con dobles y pruebas históricas. No volver a grabar hardware para publicar documentación.
- AC5: verificar ZIP, hashes de firmware y vendor, enlaces locales y remotos, patrones de secretos y metadatos; commits y rutas verificables en remoto.
- AC6: si el autor confirma eliminar AB-Displays-Lab, trasladar antes todos los proyectos, descargas, capturas y documentación; conservar su historial mediante parentesco Git y respaldo. Sustituir dependencias/enlaces activos al origen anterior. Verificar publicación completa antes de borrar; no borrar catálogo ni IDE. Si no confirma, conservar laboratorio.

## Aclaraciones
Pendiente únicamente la identificación y eliminación del laboratorio; no bloquea preparar el NAS y sus medios. La publicación y el borrado se limitan a los repositorios indicados. No se toca el IDE, otras cuentas/repositorios ni archivos de la SD.
