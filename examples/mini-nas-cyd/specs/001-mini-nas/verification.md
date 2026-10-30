> Evidencia histórica de la primera versión. La actualización QR y su binario se verifican en [acceso QR](../002-qr-access/verification.md).

# Evidencia · Mini NAS CYD

Fecha: 2026-10-30. Spec Kit aplicado manualmente antes de implementar, con constitución, especificación, plan, tareas y checklist comprometidos en Git. La ampliación posterior del autor autoriza expresamente cargar en la CYD, después de publicar la estación meteorológica.

| Criterio | Evidencia observada |
|---|---|
| AC1 | Compilación completa real con core ESP32 3.3.12, salida 0; 1.060.678 bytes de programa (80 %), 49.772 bytes de RAM estática (15 %). LCD HSPI, SD VSPI y táctil software implementados. |
| AC2 | Ocho pruebas unitarias del JavaScript real: correctas. Incluyen paginación/búsqueda, subida secuencial y vacío, duplicado, nombres/tamaño, cancelación, confirmación de borrado y SD ausente. Utilizan dobles de DOM/red: no son ejecución física del firmware. |
| AC3/AC4 | 35 static_assert C++ comprueban directamente nombres, rutas, reservados, límites de tamaño/espacio, overflow y finalización. Se ejecutan en compilación, no como tests de radio o filesystem. Streaming, temporal, rename y rechazo de reemplazo implementados; contrato físico ejecutado con resultado correcto desde PC. |
| AC5 | Carga real en COM4 finalizada con salida 0 y verificaciones de hash del transporte. STATUS por USB confirma programa MSR_MININAS v0.1 en ejecución y microSD montada: 7.736.393.728 bytes totales, 2.228.224 usados. Contraseña no escrita por serial. La foto privada muestra el LCD y el autor confirmó la calibración y los tres botones, con sus límites documentados. |
| AC6 | Fuentes, web autónoma, herramientas, pruebas y guía para el IDE, sin archivos compilados ni secretos en el paquete. Documentación y alcance revisados. |

## Carga y preservación

Se confirmó por HTTPS público que la estación meteorológica estaba en AB-MakerIDE-Projects antes de cargar el NAS. Se conservaron sus fuentes y los binarios de su compilación local como recuperación privada, sin leer flash/NVS del dispositivo, sin borrado total ni formateo de microSD. El transporte verificó la escritura y reinició la placa; la consulta serial identificó después el programa NAS, lo que aporta evidencia de arranque.

La primera compilación detectó un conflicto con BOOT_PIN definido por el core; se corrigió usando NAS_BOOT_GPIO y se repitió la compilación completa con éxito. No se arrastró un binario anterior a la carga.

## Pendiente en dispositivo

El acceso autenticado y las transferencias con checksum, duplicado, interrupción y borrado de ensayo se comprobaron desde PC en esta CYD. Se ha observado el LCD en la foto privada, TCAL=1 por USB y confirmación del autor de CLAVE/SD/CALIBRAR. Quedan pendientes Android/iOS, carga prolongada, reinicio/persistencia, cortes de alimentación y otras tarjetas. No se atribuye una prueba física exhaustiva al montaje de la SD o a los tests JavaScript.

Consulta [el ensayo de dispositivo](../../tests/HARDWARE.md). No se publica el NAS en GitHub ni se crea una release durante esta tarea; el autor solicitó su carga en la CYD.

## Corrección del síntoma de controles

Se eliminó la dependencia circular de la calibración: una pulsación nueva sin calibración la inicia, BOOT mantenido y soltado la recupera, y SHOWKEY/CALIBRATE por USB permiten diagnóstico sin imprimir la clave. Se retiró la dependencia de PENIRQ inmediatamente tras apagar el ADC y se añadió detección por flanco. El firmware corregido se compiló/cargó nuevamente. Diagnóstico real actual en results.json; la corrección táctil física solo puede considerarse confirmada si el autor la observa y lo comunica.

Las pruebas de transferencia reales desde PC al ESP32 han pasado: autenticación, token, nombres peligrosos, round-trip binario con SHA-256, no reemplazo, interrupción y borrado del archivo de ensayo. Se restauró la Wi-Fi del PC y se eliminó el perfil temporal. No equivalen a una prueba Android/iOS ni validación exhaustiva de alimentación/táctil.

Confirmación del síntoma original: el autor comunica «Sí, ya responden» después de calibrar y probar los tres botones. [Diagnóstico y cierre del bug](bug-001-controls.md).
