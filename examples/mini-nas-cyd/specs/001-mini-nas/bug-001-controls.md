# Bug 001 · recuperación de controles y diagnóstico del AP

Síntoma comunicado tras la carga: «no funciona ningun boton». LCD no verificado visualmente desde el PC; no asumir que BOOT físico y botones dibujados tienen el mismo fallo. STATUS confirma programa y SD; la red aún no se ha identificado en el escaneo del PC.

Diagnóstico de código: al omitir/agotarse la primera calibración, point() permanece inactivo y CALIBRAR depende del mismo point(), creando un camino de recuperación inaccesible desde la pantalla. Además raw() descarta un toque si PENIRQ no ha vuelto a activarse inmediatamente tras apagar el ADC: esa condición no demuestra que la coordenada sea inválida. El estado real del AP no aparece en STATUS y debe medirse antes de atribuir una causa de radio.

Plan: permitir cualquier nueva pulsación para iniciar calibración si falta; recuperar calibración con BOOT mantenido o comando USB; usar detección por flanco para no repetir botones al mantener el dedo; mostrar clave temporal durante el arranque y bajo SHOWKEY por USB sin enviarla por serial. Retirar la comprobación inmediata de PENIRQ al finalizar la lectura, conservando IRQ inicial, mediana y límites. Comprobar expresamente creación/configuración del AP y añadir diagnóstico no sensible.

Aceptación: no queda estado sin vía de recuperación; aserciones de reglas de recuperación pasan; firmware compila y se carga; STATUS informa AP y calibración. Pedir observación real de botones/táctil y probar asociación/transferencia si se obtiene la clave. No marcar como corregido físicamente solo por compilar.

Tareas: diagnóstico/especificación → implementación y comprobaciones → compilación/carga → STATUS/escaneo → confirmación del síntoma y transferencia. Sin formateo, borrado de credenciales históricas ni publicación de claves.


## Verificación del síntoma
El firmware corregido compila con salida 0 y se cargó en COM4. STATUS confirmó AP_READY=1, AP_STARTED=1, AP_MATCH=1 e IP 192.168.4.1. La calibración pasó de TCAL=0 a TCAL=1 después de los toques del autor. El autor confirmó expresamente: «Sí, ya responden» para CLAVE, SD y CALIBRAR. Se considera resuelto el síntoma comunicado en esta placa; no se extrapola a otras revisiones.

Después se accedió desde el PC al AP real: autenticación y token correctos, rechazo de rutas/nombres peligrosos, subida y descarga binaria con checksum idéntico, duplicado sin reemplazo, interrupción sin archivo final, borrado del único archivo de ensayo completo. Conexión habitual del PC restaurada y perfil temporal eliminado. La foto aportada muestra LCD, AP OK y capacidad; contiene clave de sesión y no se distribuye.
