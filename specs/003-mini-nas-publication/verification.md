# Verificación de publicación del Mini NAS
2026-10-31. Base NAS c30e47c y firmware físico 670030ad7d5b5e259274d0696dad5702c5f762f7b04f3bfb0d086aad95ab3906. Esta incorporación publica fuentes y evidencia previa; no ha vuelto a ejecutar ni grabar la placa.

- AC1: diez archivos de firmware/vendor/procedencia coinciden byte a byte con la fuente verificada; ZIP de 40 entradas con MiniNasCYD.ino y cinco cabeceras, src/ricmoo y avisos MIT. Se conservan hashes del encoder adaptado.
- AC2: tres entradas únicas en projects.json, README con guía/ZIP/galería/vídeo e IDE. Radar y meteo mantienen enlaces de origen hasta resolver la consolidación.
- AC3: cuatro capturas recortadas del vídeo original, nombres ocultos con máscaras opacas; vídeo editado revisado de 30 s, sin audio original ni menús/QR privados. QR final público al IDE. Manifiestos incluyen tiempos, recortes, tratamiento y hashes.
- AC4: compilación/carga, seis pruebas reales de SD y confirmación de subida/listado móvil se diferencian de pruebas JS con dobles y ensayo HTTP histórico. La vista previa de una placa es el contenido de un archivo, no evidencia de ejecutar NAS en esa placa.
- AC5 local: UI embebida idéntica al HTML/JS; once tests JS correctos. ZIP, hashes, enlaces locales y patrones de secretos revisados sin coincidencias. Escaneo de patrones/QR no equivale a auditoría exhaustiva. La verificación remota se registrará tras publicar.
- AC6: AB-Displays-Lab incluye también meteo; identificación/eliminación pendiente de respuesta. Ambos repositorios y NAS respaldados en bundles locales antes de cambios. No se ha borrado ningún repositorio.

[Comprobaciones](checks.json). Sin releases ni cambios al IDE. El material original íntegro y archivos privados quedan fuera del repositorio.
