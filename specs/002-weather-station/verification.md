# Verificación del catálogo · estación meteorológica

Spec Kit aplicado manualmente antes de implementar. Segundo proyecto weather-station-cyd añadido a projects.json y README; el radar permanece sin cambios. La fuente canónica es AB-Displays-Lab, con guía, ZIP y seis capturas reales de Madrid. No se duplica firmware en el catálogo.

El hash de la descarga meteorológica se contrasta con el ZIP y manifiesto del laboratorio. [Informe de compilación, dos tests HTTPS, evidencia física y límites](https://github.com/msrobotika/AB-Displays-Lab/blob/main/specs/003-weather-station/verification.md).

La verificación remota y los commits concretos se registran al cerrar la publicación. No se crean releases.

## Cierre comprobado por HTTPS anónimo

- [AB-Displays-Lab: a890c48](https://github.com/msrobotika/AB-Displays-Lab/commit/a890c484c5560f49f5603bb7bfa83a028495ff96): público, 80 archivos con ruta y blob idénticos a Git local; cero releases.
- [AB-MakerIDE-Projects: 3dd193f](https://github.com/msrobotika/AB-MakerIDE-Projects/commit/3dd193fc9d4624de462091b87c47091f1cae6545): público, 26 archivos con ruta y blob idénticos a Git local; cero releases.
- ZIP meteorológico: HTTP 200 sin autenticación, 12.990 bytes; SHA-256 coincidente con manifiesto e índice. ZIP del radar: HTTP 200 y hash original conservado.
- Seis capturas verificadas píxel a píxel; vídeo completo decodificado con audio y QR comprobado; enlaces locales revisados.
- Se contrastaron los archivos públicos y los ZIP con los identificadores privados de la red: ninguna coincidencia. Las tomas del portal están excluidas.

[Resultados comprobados](publication-results.json). Este informe registra los commits de contenido; el commit posterior solo cierra documentación y tareas. Los archivos de TikTok se entregan al autor para su publicación manual.
