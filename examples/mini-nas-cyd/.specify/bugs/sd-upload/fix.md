# Instrumentación del fallo de subida

Fecha: 2026-10-30. [Diagnóstico](assessment.md). Estado: reparación aplicada y síntoma verificado; frecuencia inicial y remontajes a 1 MHz, seis pruebas físicas correctas y subida/listado desde móvil confirmados por el autor.

## Cambios

sketch.ino conserva la escritura original de cada bloque y añade los tamaños solicitado/devuelto y errno al fallo, junto con contadores seguros en STATUS. El mensaje evita equiparar cualquier fallo a tarjeta llena. No se imprimen nombre de archivo, credenciales ni contenido.

SDTEST escribe dos archivos únicos de 262 161 bytes, con bloques de 1436 y 512 bytes respectivamente; cierra y reabre, compara cada byte del patrón determinista y retira solo los archivos creados por el ensayo. En el binario de diagnóstico se empezó a 4 MHz y, tras fallar una variante, se cerraron todos los descriptores, se remontó a 1 MHz y se repitió. La carga final comienza directamente a 1 MHz. Sin formateo, cambio de particiones ni lectura de archivos personales. Una colisión de nombre cancela el ensayo antes de escribir.

## Verificación prevista

Compilación completa para el perfil real y carga en COM4, autorización existente. SDTEST sobre la misma tarjeta; registrar stdout sanitizado y hash del programa. Después repetir la subida móvil, sin dar por probado el recorrido HTTP solo por escribir desde USB. Las pruebas JS existentes no reproducen el error físico y no sustituyen ese ensayo.

## Desviaciones

Ninguna hasta el momento. Si el diagnóstico requiere reparar el sistema de archivos o cambiar tarjeta, se preservarán los datos y se informará del bloqueo físico; no se formateará automáticamente.

## Reparación tras el diagnóstico

Se establece sdClock=1000000 al arrancar y mountCard utiliza ese valor también al pulsar SD. La prueba física inicial demostró EIO a 4 MHz y comparación byte a byte correcta en ambas variantes a 1 MHz. Se conserva el rechazo de escritura parcial; no se renombra ni se declara completo un archivo fallido. No se ha modificado el núcleo, la sesión QR ni la interfaz de subida. La prueba SDTEST no expone archivos ajenos ni credenciales.
