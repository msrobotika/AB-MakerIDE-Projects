# Diagnóstico: no se pueden subir archivos a la microSD

- Fecha: 2026-10-30, Europe/Madrid.
- Base respaldada: fdae261, árbol limpio al comenzar.
- Informe: «pero no logro subir nada a la tarjeta sd» después de confirmar apertura del panel con los dos QR.
- Proceso: Spec Kit manual, guía oficial local de reparación assess → fix → test. Constitución existente aplicable. Sin instalar extensiones ni agentes.
- Estado: informe válido, causa pendiente de reproducir; no atribuirlo todavía a SD o QR.

## Evidencia disponible

STATUS USB del binario QR cargado responde sin reiniciar: AP iniciado, microSD montada, capacidad 7 736 393 728 bytes y uso 2 228 224 bytes. Eso acredita montaje y consulta de capacidad; no demuestra escritura. El autor confirmó QR WEB y apertura del panel en móvil; no se han validado transferencias con este binario. Las pruebas anteriores de transferencia utilizaron autenticación Basic en un binario anterior.

El autor precisa el mensaje: «fallo de escritura o tarjeta llena, 0 archivos completados». Ese texto solo se genera en RAW_WRITE cuando uploadFile.write(raw.buf, raw.currentSize) no devuelve raw.currentSize. Por tanto la solicitud supera autenticación, token, validación de nombre, espacio declarado y apertura del temporal; el fallo comunicado es de escritura durante el cuerpo. No es evidencia de tarjeta llena: los contadores de capacidad indican espacio libre. Falta distinguir error físico/FAT32, tamaño/bloque y fallo del flujo. Se recoge diagnóstico USB durante un reintento.

El PC está en su conexión habitual y no alcanza 192.168.4.1. No se ha solicitado ni registrado la clave actual ni se ha cambiado de red. Se ha pedido al autor el síntoma y el mensaje exacto en el panel.

## Recorrido revisado

web/app.js valida nombre y tamaño, manda File como cuerpo binario mediante XHR al mismo origen y adjunta X-NAS-Token/X-File-Name. El navegador adjunta su cookie al mismo origen. El servidor recoge Cookie y cabeceras de subida, verifica sesión/token/SD/nombre/espacio y escribe primero un temporal propio; publica mediante rename después de tamaño completo. El parser RAW del core instalado recoge cabeceras antes del callback. No se ha probado que estas rutas funcionen desde el móvil en esta incidencia.

Hipótesis que requieren evidencia: selector de archivos en navegador integrado, nombre no admitido, sesión/token rechazados, fallo al escribir/renombrar en FAT32, desconexión o transferencia detenida. No cambiar autenticación ni formatear para eludir el síntoma.

## Criterios de aceptación

- AC1: reproducir la selección y subida desde la sesión QR y documentar el síntoma concreto.
- AC2: un archivo de prueba con nombre único y ASCII se guarda y se descarga con bytes idénticos; no se toca ningún archivo preexistente.
- AC3: errores de nombre, sesión, espacio o SD aparecen de forma comprensible; no se informa éxito antes del guardado.
- AC4: mantener QR, autenticación, token de escritura, no sobrescritura y calibración; no publicar secretos ni binarios.

## Plan y tareas

- [x] T001 Respaldar base y revisar constitución, código y proceso oficial de bugs.
- [x] T002 Consultar estado de placa sin reiniciar y pedir síntoma exacto.
- [x] T003 Reproducir y concretar causa/remediación antes de editar fuentes.
- [x] T004 Aplicar reparación mínima y pruebas de regresión pertinentes.
- [x] T005 Compilar/cargar si procede y repetir el síntoma físico; documentar límites.
- [x] T006 Actualizar paquete y evidencia sin secretos; commit local.

Coherencia: AC1→T003, AC2/3→T004/5, AC4→revisión y T006. Incógnita material: qué ocurre al elegir/subir y qué mensaje muestra el navegador. No hay diagnóstico suficiente para cambiar el firmware todavía.

## Plan de instrumentación tras reintento

El autor reproduce exactamente el error otra vez. La captura USB de 55 segundos no recibió bytes; el perfil compila con depuración del núcleo desactivada, así que ausencia de logs no descarta fallo del driver. Remediación inicial limitada a obtener evidencia: añadir contadores de subida y errno en STATUS y al fallo, y comando SDTEST para crear archivos únicos de ensayo, escribir patrones deterministas con bloques 1436 y 512, cerrar, reabrir, comprobar bytes y retirar únicamente sus archivos. Probar a 4 MHz; si hay fallo, desmontar/reinicializar a 1 MHz con todos los archivos cerrados y repetir. No anunciar arreglo ni cambiar frecuencia permanente sin resultado físico. El reinicio por carga renovará los QR; preservar calibración.

Archivos previstos: sketch.ino para diagnóstico y ensayo; nas_rules.h/nas_selftest.h solo si una reparación posterior precisa reglas nuevas; tests y documentación de esta incidencia. Verificación exigida: compilar, carga autorizada existente, ejecutar SDTEST en la tarjeta real y registrar resultados sanitizados; después concretar reparación y repetir subida móvil. No imprimir claves, cookies ni nombres de archivos del autor.

## Causa contrastada y remediación concreta

El binario de diagnóstico 709dd621d7f69eb3410cf1927592c7ed1e03d21060397b27d059bb7ea58d0ea2 reproduce un fallo de escritura sin HTTP ni autenticación: a 4 MHz la variante de bloques 1436 completa 262 161 bytes, y la de 512 se detiene en 48 640 con errno 5 (EIO). En ambos casos se retira el archivo propio. Tras remontar a 1 MHz, ambas variantes escriben y verifican los 262 161 bytes completos y limpian sus archivos. Esto acredita un fallo intermitente de E/S a 4 MHz con esta placa/tarjeta y recuperación en el ensayo a 1 MHz; no determina si su origen eléctrico está en tarjeta, lector, alimentación o señal.

Remediación aprobada por evidencia: hacer 1 MHz la frecuencia inicial y conservarla en los remontajes; mantener el control estricto de longitud y no reintentar/saltar silenciosamente un bloque fallido. Recompilar, cargar y repetir las dos pruebas físicas; después pedir nueva conexión con QR y subida desde móvil. No se promete compatibilidad universal o estabilidad prolongada por estas pruebas.

## Cierre

El autor confirma «Sí, se guarda y aparece» al repetir la subida desde el móvil con la carga corregida y comprobar la lista. El síntoma comunicado se da por reparado en esta placa/tarjeta, apoyado por seis pruebas directas de escritura/lectura completas y la observación de subidas sin escrituras parciales. [Verificación y límites](test.md). No se acredita aún hash de descarga móvil, estabilidad prolongada o cortes de alimentación.
