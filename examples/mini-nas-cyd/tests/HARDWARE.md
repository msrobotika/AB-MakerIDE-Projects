# Ensayo físico pendiente · CYD Mini NAS

No se marca ninguna casilla como superada por compilar o simular el navegador.

- [ ] Confirmar revisión, respaldo del sketch anterior, alimentación y tarjeta FAT32 de prueba con un archivo ajeno al proyecto.
- [ ] Tras una carga autorizada, calibrar táctil y comprobar LCD/SD simultáneos sin conflicto de buses. BOOT omite calibración y controla visibilidad de clave; ocultación automática a 30 s.
- [ ] Conectar un móvil al AP WPA2 sin Internet; login maker, sin acceso anónimo a API/subidas/descargas.
- [ ] Subir foto y archivo binario, comprobar tamaños y SHA-256 tras descarga, previsualización y persistencia después de reiniciar con nueva clave.
- [ ] Ejecutar tests/device_contract.py y, si se autoriza escribir en esa SD, --write-tests. Duplicado no debe modificar el original; token ausente y nombres peligrosos no deben escribir.
- [ ] Cancelar desde el móvil y cortar Wi-Fi durante una transferencia. Ningún parcial debe aparecer como archivo completo; una subida posterior debe funcionar.
- [ ] Probar tarjeta sin espacio suficiente, ausencia/lectura fallida y rechazo de archivos mayores de 128 MiB; sin formateo ni sustitución de datos existentes.
- [ ] Borrar solo el archivo confirmado; el archivo ajeno a /MSR_NAS debe conservarse. Comprobar paginación con más de 40 archivos.
- [ ] Repetir con móvil Android/iOS y marcar solo los realmente usados. Medir velocidad/estabilidad antes de publicar cifras.
- [ ] Al grabar capturas/vídeo, ocultar clave de sesión y archivos personales. No presentar vistas previas con datos ficticios como evidencia física.


## Ensayo realizado en la placa del autor
2026-10-30: carga correcta, programa identificado por USB, SD de 7.736.393.728 bytes montada, AP real y contrato de transferencia desde PC correctos. El autor completó la calibración y confirmó CLAVE/SD/CALIBRAR. Los casos combinados de la lista superior conservan sus casillas pendientes si falta cualquier parte (reinicio y persistencia, tarjetas/errores, Android/iOS, tamaños grandes y estabilidad). Los tests ejecutados no hacen pasar esas comprobaciones no realizadas.
