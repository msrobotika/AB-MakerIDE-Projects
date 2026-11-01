# Especificación: incorporar Radar WiFi CYD real
**Fecha**: 2026-10-30 | **Estado**: lista para implementar | **Rama de entrega**: main

## Historias y aceptación
### US1 / P1 — Proyecto auténtico y recuperable
- AC1: usar el repositorio existente AB-Displays-Lab privado; no crear repositorios ni releases.
- AC2: publicar sketch.ino, radar_logic.h, radar_selftest.h y las dos referencias con hashes iguales al origen; conservar README original como referencia histórica.
- AC3: README en español con funciones reales, pines, dependencias, pasos reproducibles de compilación y enlaces verificados al IDE.
### US2 / P1 — Demostración física sin datos privados
- AC4: ocho PNG derivados de fotogramas reales del vídeo aportado; indicar que son una nueva extracción y no los PNG inaccesibles de /mnt/data. Registrar cuadro, tiempo, hash y máscaras; ningún contenido sintético.
- AC5: revisión visual de los ocho PNG, SSID/BSSID y datos ajenos ocultos; no subir vídeo original, flash, secretos ni material privado.
### US3 / P1 — Evidencia y entrega verificables
- AC6: distinguir observaciones del vídeo, declaraciones de exportación, autopruebas y compilación realmente ejecutadas; no afirmar nueva carga ni verificación física completa.
- AC7: verificar rutas, enlaces internos, hashes, inventario y commits remotos; registrar limitaciones concretas.

## Aclaraciones y casos límite
La búsqueda de repositorios del propietario devuelve un único laboratorio de pantallas: AB-Displays-Lab, creado el 2026-10-30, vacío y privado. Solo AB-MakerIDE-SUPER figura público.
Los seis adjuntos y el vídeo están recuperados; el usuario también proporciona sus rutas originales. /mnt/data y el ZIP del chat anterior no existen en este entorno. Se permite extraer de nuevo ocho fotogramas del mismo vídeo y anonimizar sus identificadores, sin presentarlos como archivos idénticos al pack anterior.
El código conserva su comportamiento de mostrar identificadores en la placa; la anonimización afecta a la documentación publicada. Sin nueva licencia inventada para el código del usuario.
